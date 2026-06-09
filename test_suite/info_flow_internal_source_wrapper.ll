define i32 @read_secret_wrapper() {
  %secret = call i32 @SOURCE()
  ret i32 %secret
}

define i32 @main() {
  %out = call i32 @read_secret_wrapper()
  call void @SINK(i32 %out)
  ret i32 0
}

declare i32 @SOURCE()
declare void @SINK(i32)
