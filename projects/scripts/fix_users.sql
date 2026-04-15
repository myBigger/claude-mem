-- PrisonSIS 修复 users 表数据
-- 注意：role 必须符合 CHECK(role IN ('Admin','Legal','Interrogator','ReadOnly'))
-- 已在代码中修正，禁止使用 'Normal' 作为 role 值

DELETE FROM users WHERE username IN ('admin', 'user01', 'user02');

INSERT INTO users (user_id, username, password_hash, real_name, role, department, position, phone, enabled, failed_login_attempts, locked_until, last_login_time, created_at, remark) VALUES
('US-0001', 'admin', '2261ae7039c889223f6653792a4d2da8', '系统管理员', 'Admin', '信息科', '科长', '', 1, 0, '', '', '2026-01-01 00:00:00', '');

INSERT INTO users (user_id, username, password_hash, real_name, role, department, position, phone, enabled, failed_login_attempts, locked_until, last_login_time, created_at, remark) VALUES
('US-0002', 'user01', 'd05e67e42bcf4e2c343c1106e0527a6e', '张三', 'ReadOnly', '一监区', '管教员', '', 1, 0, '', '', '2026-01-01 00:00:00', '');

INSERT INTO users (user_id, username, password_hash, real_name, role, department, position, phone, enabled, failed_login_attempts, locked_until, last_login_time, created_at, remark) VALUES
('US-0003', 'user02', '11290d4bd041ac0268f914af0eaac72c', '李四', 'ReadOnly', '二监区', '管教员', '', 1, 0, '', '', '2026-01-01 00:00:00', '');
