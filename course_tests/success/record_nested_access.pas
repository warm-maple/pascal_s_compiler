program RecordNestedAccess(input, output);
var
    point : record
        x : integer;
        inner : record
            y : integer;
        end;
    end;
begin
    point.x := 3;
    point.inner.y := point.x + 4;
    write(point.inner.y);
end.
