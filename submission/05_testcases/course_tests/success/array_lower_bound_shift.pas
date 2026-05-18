program ArrayLowerBoundShift(input, output);
var
    arr : array[3..5] of integer;
begin
    arr[3] := 1;
    arr[4] := 2;
    arr[5] := 3;
    write(arr[3], arr[4], arr[5]);
end.
