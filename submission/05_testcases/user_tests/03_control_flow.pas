// 测试语法分析：控制流结构嵌套
program ControlFlowTest(input, output);
var
    i, sum : integer;
begin
    sum := 0;
    
    for i := 1 to 10 do
    begin
        if i mod 2 = 0 then
            sum := sum + i
        else
            sum := sum - i;
    end;
    
    while sum < 100 do
        sum := sum + 10;
        
    write(sum);
end.
