if not exist "packed" mkdir packed
xcopy /s /d /y build\* packed
xcopy /s /d /y prereqs\* packed