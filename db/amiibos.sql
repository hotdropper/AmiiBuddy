-- auto-generated definition
create table amiibos
(
    id           integer not null
        constraint amiibos_pk
            primary key autoincrement,
    directory_id integer not null default 0,
    name         text,
    amiibo_name  text,
    owner_name   text,
    variation    integer,
    form         integer,
    number       integer,
    "set"        integer,
    head         blob,
    tail         blob,
    file         text,
    hash         text,
    last_updated integer
);

create index amiibos_directory_id_name_index
    on amiibos (directory_id, name);

create index amiibos_file_index
    on amiibos (file);

create unique index amiibos_hash_index
    on amiibos (hash);

create unique index amiibos_id_uindex
    on amiibos (id);

