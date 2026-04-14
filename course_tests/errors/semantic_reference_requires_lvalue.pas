program SemanticReferenceRequiresLValue(input, output);

procedure IncByOne(var x: integer);
begin
    x := x + 1;
end;

begin
    IncByOne(1 + 2);
end.
