[Tasks]
Name: envpath; Description: "Add p++ to user PATH"; Flags: unchecked

[Registry]
Root: HKCU; Subkey: "Environment"; ValueType: expandsz; ValueName: "Path"; ValueData: "{olddata};{app}\bin"; Tasks: envpath; Flags: uninsdeletevalue
