.headers on
.mode column
create table emp(id integer primary key, name text, dept text, salary real);
insert into emp(name, dept, salary) values
 ('Ada', 'R&D', 9200.50), ('Grace', 'R&D', 8800), ('Linus', 'Ops', 7100),
 ('Ken', 'Ops', 6900.25), ('Barbara', 'Sales', 7500);
select dept, count(*) as n, round(avg(salary), 2) as avg_sal,
 max(salary) as top from emp group by dept order by dept;
create index emp_dept on emp(dept);
begin;
update emp set salary = salary * 1.10 where dept = 'Ops';
delete from emp where name = 'Barbara';
commit;
select name, printf('%.2f', salary) as salary from emp order by salary desc;
with recursive n(x) as (select 1 union all select x + 1 from n where x < 10)
select sum(x) as sum_1_10, group_concat(x, '') as digits from n;
select sqlite_version() >= '3.46' as v_ok, length(randomblob(16)) as rb;
