define i32 @helper(i32 %x) {
  %y = add i32 %x, 1
  ret i32 %y
}

define i32 @main() {
  %secret = call i32 @SOURCE()
  %out = call i32 @helper(i32 %secret)
  call void @SINK(i32 %out)
  ret i32 0
}

declare i32 @SOURCE()
declare void @SINK(i32)
