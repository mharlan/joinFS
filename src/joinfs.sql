DROP TABLE IF EXISTS test_table;
DROP TABLE IF EXISTS files;
DROP TABLE IF EXISTS symlinks;
DROP TABLE IF EXISTS queries;

CREATE TABLE test_table(id INTEGER PRIMARY KEY,
	   		 			name TEXT NOT NULL);

INSERT INTO test_table VALUES(1, "1");
INSERT INTO test_table VALUES(2, "2");
INSERT INTO test_table VALUES(3, "3");
INSERT INTO test_table VALUES(4, "4");
INSERT INTO test_table VALUES(5, "5");
INSERT INTO test_table VALUES(6, "6");
INSERT INTO test_table VALUES(7, "7");
INSERT INTO test_table VALUES(8, "8");
INSERT INTO test_table VALUES(9, "9");
INSERT INTO test_table VALUES(10, "10");
INSERT INTO test_table VALUES(11, "11");
INSERT INTO test_table VALUES(12, "12");
INSERT INTO test_table VALUES(13, "13");
INSERT INTO test_table VALUES(14, "14");
INSERT INTO test_table VALUES(15, "15");
INSERT INTO test_table VALUES(16, "16");
INSERT INTO test_table VALUES(17, "17");
INSERT INTO test_table VALUES(18, "18");
INSERT INTO test_table VALUES(19, "19");
INSERT INTO test_table VALUES(20, "20");
INSERT INTO test_table VALUES(21, "21");
INSERT INTO test_table VALUES(22, "22");
INSERT INTO test_table VALUES(23, "23");
INSERT INTO test_table VALUES(24, "24");
INSERT INTO test_table VALUES(25, "25");
INSERT INTO test_table VALUES(26, "26");
INSERT INTO test_table VALUES(27, "27");
INSERT INTO test_table VALUES(28, "28");
INSERT INTO test_table VALUES(29, "29");
INSERT INTO test_table VALUES(30, "30");

CREATE TABLE files(fileid INTEGER PRIMARY KEY AUTOINCREMENT, 
	   		 	   inode INTEGER NOT NULL,
				   access INTEGER NOT NULL,
				   datapath TEXT NOT NULL,
				   filename TEXT NOT NULL);

CREATE TABLE symlinks(syminode INTEGER PRIMARY KEY NOT NULL,
					  datainode INTEGER NOT NULL,
					  fileid INTEGER,
					  FOREIGN KEY(fileid) REFERENCES files(fileid));

CREATE TABLE queries(qinode INTEGER PRIMARY KEY,
	   		 	     query TEXT);