define void @copy_to_ptr(i32 %x, ptr %out) {
  store i32 %x, ptr %out
  ret void
}

define i32 @main() {
  %slot = alloca i32
  %secret = call i32 @SOURCE()
  call void @copy_to_ptr(i32 %secret, ptr %slot)
  %out = load i32, ptr %slot
  call void @SINK(i32 %out)
  ret i32 0
}

declare i32 @SOURCE()
declare void @SINK(i32)
