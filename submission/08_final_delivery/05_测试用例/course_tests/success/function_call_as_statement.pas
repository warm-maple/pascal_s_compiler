program FunctionCallAsStatement(input, output);
var
    a : integer;

function inc_a : integer;
begin
    a := a + 1;
    inc_a := a;
end;

begin
    a := 0;
    inc_a;
    write(a, inc_a);
end.
