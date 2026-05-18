program RepeatUntilBasic(input, output);
var
    i: integer;
    sum: integer;
begin
    i := 1;
    sum := 0;
    repeat
        sum := sum + i;
        i := i + 1
    until i > 3;
    write(sum);
end.
