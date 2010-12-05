DROP TABLE IF EXISTS files;
DROP TABLE IF EXISTS symlinks;
DROP TABLE IF EXISTS queries;

CREATE TABLE files(fileid INTEGER PRIMARY KEY AUTOINCREMENT, 
	   		 	   inode INTEGER NOT NULL,
				   access INTEGER NOT NULL,
				   datapath TEXT);

CREATE TABLE symlinks(syminode INTEGER PRIMARY KEY,
					  linkinode INTEGER,
					  fileid INTEGER,
					  FOREIGN KEY(fileid) REFERENCES files(fileid));

CREATE TABLE queries(qinode INTEGER PRIMARY KEY,
	   		 	     query TEXT);