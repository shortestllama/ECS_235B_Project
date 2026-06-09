define i32 @choose_constant(i32 %x) {
  %cond = icmp sgt i32 %x, 0
  br i1 %cond, label %then, label %else

then:
  ret i32 1

else:
  ret i32 0
}

define i32 @main() {
  %secret = call i32 @SOURCE()
  %out = call i32 @choose_constant(i32 %secret)
  call void @SINK(i32 %out)
  ret i32 0
}

declare i32 @SOURCE()
declare void @SINK(i32)
