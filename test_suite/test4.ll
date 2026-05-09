define i32 @main() #0 {
begin:
  %aVar = alloca i32
  store i32 0, ptr %aVar
  %secret = call i32 () @SOURCE()
  br label %loop_start
loop_start:
  %curr = load i32, ptr %aVar
  %new = add i32 %curr, 1
  store i32 %new, ptr %aVar
  %loop = icmp ne i32 %new, 10
  br i1 %loop, label %loop_start, label %loop_end
loop_end:
  call void @SINK(i32 %secret)
  ret i32 0
}

declare i32 @SOURCE()
declare void @SINK(i32)