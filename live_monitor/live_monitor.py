"""Read local Windows failed-logon audit events and show review alerts.

This is a passive monitor: it never blocks accounts, changes firewall rules,
clears logs, or sends event data to another service.
"""
from __future__ import annotations

import os
import re
import sqlite3
import subprocess
import sys
import tkinter as tk
from contextlib import closing
import xml.etree.ElementTree as ET
from collections import Counter
from datetime import datetime, timedelta, timezone
from pathlib import Path
from tkinter import messagebox, ttk

APP_DIR = Path(os.environ.get("LOCALAPPDATA", Path.home())) / "LoginShieldNG" / "LiveMonitor"
DB_PATH = APP_DIR / "events.sqlite3"
POLL_SECONDS = 15
WINDOW_MINUTES = 5
ALERT_THRESHOLD = 10
MAX_EVENTS_PER_QUERY = 250
NS = {"e": "http://schemas.microsoft.com/win/2004/08/events/event"}


def parse_event_xml(xml_text: str) -> list[dict]:
    events=[]
    fragments=re.findall(r"<Event\b.*?</Event>",xml_text,re.DOTALL)
    for fragment in fragments:
        try:root=ET.fromstring(fragment)
        except ET.ParseError:continue
        sys_node=root.find("e:System",NS)
        if sys_node is None:continue
        eid=sys_node.findtext("e:EventID",default="",namespaces=NS)
        if eid!="4625":continue
        record_id=sys_node.findtext("e:EventRecordID",default="",namespaces=NS)
        time_node=sys_node.find("e:TimeCreated",NS)
        created=(time_node.attrib.get("SystemTime","") if time_node is not None else "")
        data={item.attrib.get("Name",""):item.text or "" for item in root.findall("e:EventData/e:Data",NS)}
        events.append({"record_id":record_id,"created_at":created,"username":data.get("TargetUserName", ""),
            "domain":data.get("TargetDomainName", ""),"ip":data.get("IpAddress", ""),
            "workstation":data.get("WorkstationName", ""),"logon_type":data.get("LogonType", ""),
            "status":data.get("Status", ""),"sub_status":data.get("SubStatus", "")})
    return events


def query_security_log(limit: int = MAX_EVENTS_PER_QUERY) -> list[dict]:
    if sys.platform != "win32":raise OSError("Live Security log access is available only on Windows.")
    wevtutil=Path(os.environ.get("SystemRoot",r"C:\Windows")) / "System32" / "wevtutil.exe"
    query="*[System[(EventID=4625)]]"
    args=[str(wevtutil),"qe","Security",f"/q:{query}","/rd:true","/f:xml",f"/c:{limit}"]
    proc=subprocess.run(args,capture_output=True,text=True,encoding="utf-8",errors="replace",timeout=15,check=False)
    if proc.returncode:
        detail=(proc.stderr or proc.stdout).strip()
        if "access" in detail.lower() or "denied" in detail.lower():
            raise PermissionError("Windows denied access to the Security log. Run under an account allowed to read it (for example, an authorized Event Log Readers member) and try again.")
        raise RuntimeError(detail or f"wevtutil exited with code {proc.returncode}.")
    return parse_event_xml(proc.stdout)


class EventStore:
    def __init__(self,path:Path=DB_PATH):
        self.path=path;self.path.parent.mkdir(parents=True,exist_ok=True)
        with closing(self.connect()) as db:
            db.executescript("""
            CREATE TABLE IF NOT EXISTS events(record_id TEXT PRIMARY KEY,created_at TEXT NOT NULL,username TEXT,domain TEXT,ip TEXT,workstation TEXT,logon_type TEXT,status TEXT,sub_status TEXT);
            CREATE INDEX IF NOT EXISTS ix_events_created ON events(created_at);
            """)
            cutoff=(datetime.now(timezone.utc)-timedelta(days=30)).isoformat()
            db.execute("DELETE FROM events WHERE created_at < ?",(cutoff,))

    def connect(self):
        db=sqlite3.connect(self.path,timeout=10);db.row_factory=sqlite3.Row;return db

    def add(self,events:list[dict])->int:
        inserted=0
        with closing(self.connect()) as db:
            for event in events:
                if not event.get("record_id") or not event.get("created_at"):continue
                cursor=db.execute("INSERT OR IGNORE INTO events VALUES(?,?,?,?,?,?,?,?,?)",tuple(event.get(k,"") for k in ("record_id","created_at","username","domain","ip","workstation","logon_type","status","sub_status")))
                inserted+=cursor.rowcount
        return inserted

    def recent(self,limit:int=250):
        with closing(self.connect()) as db:return db.execute("SELECT * FROM events ORDER BY created_at DESC LIMIT ?",(limit,)).fetchall()

    def alerts(self):
        cutoff=(datetime.now(timezone.utc)-timedelta(minutes=WINDOW_MINUTES)).isoformat()
        recent=[e for e in self.recent(1000) if e["created_at"]>=cutoff]
        by_ip=Counter(e["ip"] for e in recent if e["ip"] and e["ip"] not in ("-","::1","127.0.0.1"))
        by_user=Counter(e["username"].lower() for e in recent if e["username"] and e["username"] not in ("-","ANONYMOUS LOGON"))
        alerts=[]
        alerts.extend(("source",value,count) for value,count in by_ip.items() if count>=ALERT_THRESHOLD)
        alerts.extend(("account",value,count) for value,count in by_user.items() if count>=ALERT_THRESHOLD)
        return sorted(alerts,key=lambda x:(-x[2],x[0],x[1]))


def run_self_test():
    sample='''<Event xmlns="http://schemas.microsoft.com/win/2004/08/events/event"><System><EventID>4625</EventID><EventRecordID>7001</EventRecordID><TimeCreated SystemTime="2026-10-30T10:00:00.000Z" /></System><EventData><Data Name="TargetUserName">sample-user</Data><Data Name="IpAddress">203.0.113.10</Data><Data Name="WorkstationName">DEMO-PC</Data><Data Name="LogonType">3</Data><Data Name="Status">0xC000006D</Data><Data Name="SubStatus">0xC000006A</Data></EventData></Event>'''
    parsed=parse_event_xml(sample)
    assert len(parsed)==1 and parsed[0]["record_id"]=="7001" and parsed[0]["ip"]=="203.0.113.10"
    assert parse_event_xml(sample.replace("<EventID>4625</EventID>","<EventID>4624</EventID>"))==[]
    print("Self-check passed: failed-logon XML parsed; successful-logon IDs ignored.")


class MonitorApp:
    def __init__(self,root:tk.Tk,store:EventStore,demo:bool=False):
        self.root,self.store,self.demo=root,store,demo;self.running=True;self.error_shown=False
        root.title("LoginShield NG · Windows event monitor"+(" · DEMO" if demo else ""));root.geometry("1080x660");root.minsize(850,500)
        self.build();self.refresh(first=True)

    def build(self):
        style=ttk.Style()
        try:style.theme_use("vista")
        except tk.TclError:pass
        style.configure("Treeview",rowheight=26,font=("Segoe UI",9));style.configure("Treeview.Heading",font=("Segoe UI",9,"bold"))
        head=ttk.Frame(self.root,padding=(18,14));head.pack(fill="x")
        ttk.Label(head,text="LOGINSHIELD NG",font=("Segoe UI",17,"bold")).pack(side="left")
        ttk.Label(head,text="Passive Windows sign-in monitor",foreground="#65756b").pack(side="left",padx=12)
        ttk.Button(head,text="Check now",command=self.check_now).pack(side="right")
        self.status=ttk.Label(self.root,text="Waiting for first Security log check…",padding=(19,0,19,8));self.status.pack(anchor="w")
        cards=ttk.Frame(self.root,padding=(18,0));cards.pack(fill="x")
        self.count=ttk.Label(cards,text="0",font=("Segoe UI",20,"bold"));self.count.grid(row=0,column=0,sticky="w",padx=(0,45))
        ttk.Label(cards,text=f"Failed logons stored locally · last {WINDOW_MINUTES} minutes",font=("Segoe UI",9)).grid(row=1,column=0,sticky="w",padx=(0,45))
        self.alert_summary=ttk.Label(cards,text="No threshold alerts",font=("Segoe UI",11,"bold"),foreground="#356b48");self.alert_summary.grid(row=0,column=1,rowspan=2,sticky="w")
        box=ttk.LabelFrame(self.root,text="Recent failed logon events · Windows Event ID 4625",padding=10);box.pack(fill="both",expand=True,padx=18,pady=14)
        cols=("time","account","source","workstation","type","reason")
        self.table=ttk.Treeview(box,columns=cols,show="headings")
        for col,title,width in [("time","TIME (UTC)",174),("account","ACCOUNT",150),("source","SOURCE IP",130),("workstation","WORKSTATION",160),("type","LOGON TYPE",90),("reason","STATUS / SUBSTATUS",220)]:self.table.heading(col,text=title);self.table.column(col,width=width,anchor="w")
        self.table.pack(fill="both",expand=True)
        foot=ttk.Frame(self.root,padding=(18,0,18,13));foot.pack(fill="x")
        ttk.Label(foot,text="Events stay on this PC and are retained for up to 30 days. This app never blocks accounts or changes Windows settings.",foreground="#69766e").pack(side="left")
        self.root.protocol("WM_DELETE_WINDOW",self.close)

    def check_now(self):
        try:
            events=self.sample() if self.demo else query_security_log()
            added=self.store.add(events)
            self.error_shown=False
            self.status.configure(text=f"Last check: {datetime.now().astimezone().strftime('%Y-%m-%d %H:%M:%S %Z')} · {added} new events · refreshes every {POLL_SECONDS} seconds")
            self.render()
        except (OSError,RuntimeError,PermissionError,subprocess.TimeoutExpired) as err:
            self.status.configure(text="Could not read Windows Security log.")
            if not self.error_shown:
                messagebox.showwarning("Security log unavailable",str(err)+"\n\nThe app has not changed Windows auditing or permissions.",parent=self.root)
                self.error_shown=True
        if self.running:self.root.after(POLL_SECONDS*1000,self.check_now)

    def sample(self):
        now=datetime.now(timezone.utc).isoformat(timespec="milliseconds").replace("+00:00","Z")
        return [{"record_id":str(880000+i),"created_at":now,"username":"sample-user","domain":"DEMO","ip":"203.0.113.24","workstation":"DEMO-PC","logon_type":"3","status":"0xC000006D","sub_status":"0xC000006A"} for i in range(12)]

    def refresh(self,first=False):
        if first and self.demo:self.check_now()
        elif first:self.root.after(500,self.check_now)

    def render(self):
        rows=self.store.recent(250);self.count.configure(text=str(len(rows)))
        for node in self.table.get_children():self.table.delete(node)
        for e in rows:self.table.insert("","end",values=(e["created_at"].replace("T"," ").replace("Z",""),e["username"] or "(unknown)",e["ip"] or "—",e["workstation"] or "—",e["logon_type"] or "—",f"{e['status']} / {e['sub_status']}"))
        alerts=self.store.alerts()
        if alerts:
            labels=[f"{kind} {value}: {count} in {WINDOW_MINUTES} min" for kind,value,count in alerts[:3]]
            self.alert_summary.configure(text="Review: "+" · ".join(labels),foreground="#a25d18")
        else:self.alert_summary.configure(text="No threshold alerts",foreground="#356b48")

    def close(self):self.running=False;self.root.destroy()


def main():
    if "--self-test" in sys.argv:run_self_test();return 0
    demo="--demo" in sys.argv
    if not demo and sys.platform!="win32":print("Live monitoring requires Windows; use --demo elsewhere.");return 2
    path=APP_DIR/("demo-events.sqlite3" if demo else "events.sqlite3")
    root=tk.Tk();MonitorApp(root,EventStore(path),demo=demo);root.mainloop();return 0


if __name__=="__main__":raise SystemExit(main())
