define i32 @main() {
  %public_slot = alloca i32
  %secret = call i32 @SOURCE()
  %cond = icmp sgt i32 %secret, 0
  br i1 %cond, label %then, label %else

then:
  br label %merge

else:
  br label %merge

merge:
  store i32 5, ptr %public_slot
  %out = load i32, ptr %public_slot
  call void @SINK(i32 %out)
  ret i32 0
}

declare i32 @SOURCE()
declare void @SINK(i32)
