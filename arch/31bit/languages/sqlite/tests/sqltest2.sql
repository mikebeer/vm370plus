select count(*) as rows_after_reopen, round(sum(salary), 2) as total from emp;
pragma integrity_check;
