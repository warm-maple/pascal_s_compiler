// 测试语法分析：一维数组和多维数组的声明及访问
program ArrayTest(input, output);
var
    arr1 : array[1..10] of integer;
    arr2 : array[1..5, 1..5] of real;
    i : integer;
begin
    // 初始化一维数组
    for i := 1 to 10 do
        arr1[i] := i * 10;
        
    // 测试二维数组访问
    arr2[1, 1] := 3.14;
    arr2[5, 5] := arr1[1] + arr2[1, 1];
    
    write(arr1[5]);
end.
