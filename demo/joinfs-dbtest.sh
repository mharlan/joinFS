ls -l -i data/
ls -l -i queries/
sqlite3 joinfs.db 'select * from files;'
sqlite3 joinfs.db 'select * from symlinks;'
sqlite3 joinfs.db 'select syminode, inode, filename, datapath from files, symlinks where datainode=inode;'
sqlite3 joinfs.db 'select * from keys;'
sqlite3 joinfs.db 'select * from metadata;'