define void @fill_secret(ptr %out) {
  %secret = call i32 @SOURCE()
  store i32 %secret, ptr %out
  ret void
}

define i32 @main() {
  %slot = alloca i32
  call void @fill_secret(ptr %slot)
  %out = load i32, ptr %slot
  call void @SINK(i32 %out)
  ret i32 0
}

declare i32 @SOURCE()
declare void @SINK(i32)
