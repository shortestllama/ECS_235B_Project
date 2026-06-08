define i32 @main() {
  %x = alloca i32
  %secret = call i32 @SOURCE()
  %cond = icmp sgt i32 %secret, 0
  br i1 %cond, label %then, label %else

then:
  store i32 1, ptr %x
  br label %merge

else:
  store i32 0, ptr %x
  br label %merge

merge:
  %out = load i32, ptr %x
  call void @SINK(i32 %out)
  ret i32 0
}

declare i32 @SOURCE()
declare void @SINK(i32)
