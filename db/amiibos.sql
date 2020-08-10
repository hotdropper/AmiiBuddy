CREATE TABLE IF NOT EXISTS amiibos
(
	id integer not null
		constraint amiibos_pk
			primary key autoincrement,
    name text,
    amiibo_name text,
	owner_name text,
	variation integer,
	form integer,
    number integer,
    "set" integer,
	head blob,
	tail blob,
	file text,
	hash text,
	last_written integer
);

create unique index amiibos_hash_index
	on amiibos (hash);

create unique index amiibos_id_uindex
	on amiibos (id);

create index amiibos_file_index
    on amiibos (file);
