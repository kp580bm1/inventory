DROP TABLE IF EXISTS Places;
DROP TABLE IF EXISTS ItemTypes;
DROP TABLE IF EXISTS Items;
DROP TABLE IF EXISTS History;
DROP TABLE IF EXISTS Log;

CREATE TABLE Places (
	id INTEGER PRIMARY KEY,
	name TEXT NOT NULL
);

CREATE TABLE ItemTypes (
	id INTEGER PRIMARY KEY,
	name TEXT NOT NULL
);

CREATE TABLE Items (
	id INTEGER PRIMARY KEY,
	idItemType INTEGER NOT NULL,
	name TEXT NOT NULL,
	note TEXT NOT NULL,
	inn INTEGER NULL,
	idPlace INTEGER NOT NULL,
	active BOOLEAN NOT NULL,

	FOREIGN KEY(idItemType) REFERENCES ItemTypes(id),
	FOREIGN KEY(idPlace) REFERENCES Places(id)
);

CREATE TABLE History (
	id INTEGER PRIMARY KEY,
	eventDateTime TEXT NOT NULL,
	idItem INTEGER NOT NULL,
	changedField INTEGER NOT NULL,
	oldValue TEXT NOT NULL,
	newValue TEXT NOT NULL,

	FOREIGN KEY(idItem) REFERENCES Items(id)
);

CREATE TABLE Log (
	id INTEGER PRIMARY KEY,
	logDateTime TEXT NOT NULL,
	logText TEXT NOT NULL
);

/* @todo: triggers and views*/
