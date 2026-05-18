program VarParameterArrayElement(input, output);
var
    arr : array[0..1] of integer;

procedure swap(var x, y : integer);
var
    t : integer;
begin
    t := x;
    x := y;
    y := t;
end;

begin
    arr[0] := 1;
    arr[1] := 9;
    swap(arr[0], arr[1]);
    write(arr[0], arr[1]);
end.
