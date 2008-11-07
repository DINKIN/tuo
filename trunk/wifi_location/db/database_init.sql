drop database wifi_location;

create database wifi_location;

use wifi_location;

drop table record;
create table record (
	record_index BIGINT KEY AUTO_INCREMENT, 
	bssid INT(6),
	str_bssid CHAR(18),
	longitude DOUBLE,
	latitude DOUBLE,
	snr DOUBLE,
	ssid CHAR(30),
	channel SMALLINT,
	speed SMALLINT COMMENT 'unit is Mbps',
	vender CHAR(20),
	type BIT COMMENT '1 is AP and 0 is Ad-hoc',
	encryption CHAR(10),
	beacon_interval INT,
	time TIME,
	date DATE,
	available BOOL,
	source_index INT
	);

drop table data_source;
create table data_source (
	source_index INT KEY AUTO_INCREMENT,
	filename CHAR(100),
	collect_type INT COMMENT '1 walk, 2 drive, 3 bicycle ...',
	collecter CHAR(50), 
	date DATE
	);

drop table fingerprint;
create table fingerprint (
	fp_index INT KEY AUTO_INCREMENT,
	longitude DOUBLE,
	latitude DOUBLE
	);

drop table fingerprint_record;
create table fingerprint_record (
	fp_index INT,
	record_index INT
	);

drop table ap;
create table ap (
	ap_index INT KEY  AUTO_INCREMENT,
	str_bssid CHAR(18),
	longitude DOUBLE,
	latitude DOUBLE
	);

drop table ap_fingerprint;
create table ap_fingerprint (
	ap_index INT,
	fp_index INT
	);
