create table directories
(
    id integer not null
        constraint directories_pk
            primary key autoincrement,
    parent_id integer not null default 0
        constraint directories_directories_id_fk
            references directories
            on delete restrict,
    name text not null,
    path text not null
);

create unique index directories_id_uindex
    on directories (id);

create unique index directories_parent_id_name_uindex
    on directories (parent_id, name);

