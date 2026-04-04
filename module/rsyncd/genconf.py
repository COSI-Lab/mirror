header = """# This is a generated file. Do not edit manually.
uid = nobody
gid = nogroup
use chroot = yes
max connections = 0
pid file = /var/run/rsyncd.pid
motd file = /etc/rsyncd.motd
log file = /dev/stdout
log format = %t %o %a %m %f %b
dont compress = *.gz *.tgz *.zip *.z *.Z *.rpm *.deb *.bz2 *.tbz2 *.xz *.txz *.rar
refuse options = checksum delete

"""

body = """[{}]
    comment = {}
    path = {}
    exclude = lost+found/
    read only = true
    ignore nonreadable = yes
"""

import json

# open and deserialize mirrors.json
try:
    mj = open("mirrors.json","r")
except OSError:
    print("Error opening mirrors.json. Exiting.")
    exit(1)
try:
    mirrors : dict[str,dict] = json.load(mj)["mirrors"]
except KeyError:
    print("Error deserializing mirrors.json. Exiting.")
    exit(2)
mj.close()

# open the rsyncd.conf file
try:
    dc = open("/etc/rsyncd.conf","w")
except OSError:
    print("error opening rsyncd.conf. Exiting.")
    exit(3)

# write out the header
dc.write(header)

# write out the entries
for k,v in mirrors.items():
    mod_name = k
    if "rsync" in v.keys():
        path = v["rsync"]["dest"]
    elif "static" in v.keys():
        path = v["static"]["location"]
    else: # `command` sync types don't explicitly store a location but we can guess
        path = f"/storage/{mod_name}"
    comment = v["name"]

    entry = body.format(mod_name,comment,path)
    dc.write(entry)

# close rsyncd.conf
dc.close

exit(0)
