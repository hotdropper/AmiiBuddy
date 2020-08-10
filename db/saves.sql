CREATE TABLE IF NOT EXISTS saves
(
	id integer not null
		constraint saves_pk
			primary key autoincrement,
	amiibo_id integer not null
		constraint saves_amiibos_id_fk
			references amiibos,
	hash text not null,
    name text not null,
    file text not null,
	last_update integer not null,
    is_custom integer default 1
);

create index saves_amiibo_id_last_update_index
    on saves (amiibo_id asc, last_update desc);

create unique index saves_hash_uindex
	on saves (hash);

create unique index saves_id_uindex
	on saves (id);

