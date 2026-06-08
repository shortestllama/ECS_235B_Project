define i32 @main() {
  %secret = call i32 @SOURCE()
  %copy = add i32 %secret, 0
  call void @SINK(i32 %copy)
  ret i32 0
}

declare i32 @SOURCE()
declare void @SINK(i32)
