// 测试语法分析：表达式优先级和括号嵌套
program ExprTest(input, output);
var
    x, y, z : integer;
    res : real;
begin
    x := 5;
    y := 10;
    z := 2;
    // 应当解析为 x + (y * z)
    res := x + y * z;
    
    // 带括号的复杂表达式
    res := (x + y) * (z - 1) / 2.0;

    write(res);
end.
