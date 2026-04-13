// 测试语法分析：函数声明和调用
program FunctionTest(input, output);
var
    result : integer;

function add(a, b : integer) : integer;
var
    temp : integer;
begin
    temp := a + b;
    add := temp;
end;

begin
    result := add(5, 7);
    write(result);
end.
