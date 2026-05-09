define i32 @main() #0 {
begin:
  %secret = call i32 () @SOURCE()
  %ptr = alloca i32 
  %newptr = getelementptr i32, i32* %ptr, i64 1
  store i32 0, ptr %newptr
  %res = load i32, ptr %newptr
  %loop = icmp ne i32 %res, 10
  br i1 %loop, label %ifBody, label %afterIf
ifBody:
  %addr = add i32 100, %secret
  %subr = sub i32 100, %addr
  %divr = div i32 %subr, 10
  %mulr = mul i32 %divr, %subr
  br label %afterIf
afterIf:
  %phi = phi i32 [%res, %begin], [%mulr, %ifBody]
  call void @SINK(i32 %phi)
  ret i32 %res
}

declare i32 @SOURCE()
declare void @SINK(i32)