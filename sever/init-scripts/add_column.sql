alter table iotplus.user_permissions
    add number varchar(30) not null;

comment on column iotplus.user_permissions.number is '学号或其他标识号';


alter table iotplus.user_permissions
    add constraint number
        unique (number);

