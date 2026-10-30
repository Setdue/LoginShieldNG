# Public dataset attribution

`public_honeypot_login_events.csv` is a small conversion of the `cowrie.json.2025-04-03.anon` file from the [Salty Seagull Dataset](https://zenodo.org/records/18088113), DOI [10.5281/zenodo.18088113](https://doi.org/10.5281/zenodo.18088113). The dataset contains recorded interactions with a research VSAT honeynet. Its authors state that source IP addresses in the released files were anonymized using cryptopANT.

The CSV keeps only Cowrie `cowrie.login.failed` and `cowrie.login.success` events and maps them to LoginShield's `time,username,ip,device,status` format. This selected day contains 196 failed login attempts, 8 anonymized source IPs, 23 usernames, and one sensor identifier. It has no successful login attempts.

Field mapping:

- `time`: UTC timestamp reduced to `HH:MM:SS` because LoginShield's importer expects that format. The source file covers one date, April 3, 2025.
- `username`: attempted username from the login event.
- `ip`: anonymized `src_ip` from the dataset.
- `device`: anonymized Cowrie sensor identifier.
- `status`: Cowrie login event outcome (`failed` or `success`).

Password fields, session IDs, and command events are excluded. For the source description and requested citation, see the Zenodo record and the authors' [Salty Seagull paper](https://arxiv.org/abs/2508.11325). Check the source record for its current reuse terms.

