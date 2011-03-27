DROP TABLE IF EXISTS test_table;
DROP TABLE IF EXISTS links;
DROP TABLE IF EXISTS keys;
DROP TABLE IF EXISTS metadata;

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

CREATE TABLE links(jfs_id INTEGER PRIMARY KEY AUTOINCREMENT,
                   inode INTEGER NOT NULL,
	   		       path TEXT NOT NULL UNIQUE,
				   filename TEXT NOT NULL);

CREATE TABLE keys(keyid INTEGER PRIMARY KEY AUTOINCREMENT,
	              keytext TEXT UNIQUE NOT NULL);

CREATE TABLE metadata(jfs_id INTEGER NOT NULL,
					  keyid INTEGER NOT NULL,
					  keyvalue TEXT NOT NULL,
					  FOREIGN KEY(jfs_id) REFERENCES files(jfs_id) ON DELETE CASCADE ON UPDATE CASCADE,
					  FOREIGN KEY(keyid) REFERENCES keys(keyid) ON DELETE RESTRICT ON UPDATE CASCADE,
					  PRIMARY KEY(jfs_id, keyid));
