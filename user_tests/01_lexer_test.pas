// 测试词法分析器：大小写不敏感、注释过滤
PROGRAM LexerTest(input, output);
CONST
    PI = 3.14159;
VAR
    aB_c : integer; { 混合大小写标识符 }
    (* 
       多行注释测试
       should be ignored
    *)
BEGIN
    aB_c := 10;
    WRITELN(aB_c);
end.
