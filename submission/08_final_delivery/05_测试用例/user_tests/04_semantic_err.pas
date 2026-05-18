// 测试语义分析：故意写错，看能不能测出符号表报错
program SemanticErrTest(input, output);
var
    a : integer;
    b : real;
begin
    a := 10;
    c := 20;  // 错误：c 未声明
    
    // a := b; // 类型不匹配（目前 codegen 还不完善，先不测这个，测未声明就行）
    
    undeclared_func(a); // 错误：函数未声明
end.
