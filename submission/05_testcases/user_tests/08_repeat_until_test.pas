program RepeatUntilDemo(input, output);
var
    n: integer;
begin
    n := 0;
    repeat
        n := n + 2
    until n >= 6;
    write(n);
end.
