define i32 @main() #0 {
entry:
  %aVar = alloca i32
  %bVar = alloca i32
  %cVar = alloca i32
  %idxVar = alloca i64
  %secret = call i32 () @SOURCE()
  store i32 0, ptr %aVar
  store i32 %secret, ptr %bVar
  store i32 1, ptr %cVar
  store i64 0, ptr %idxVar
  %a0 = load i32, ptr %aVar
  %entryCond = icmp eq i32 %a0, 0
  br i1 %entryCond, label %loop_header, label %early_exit
loop_header:
  %i = load i64, ptr %idxVar
  %a1 = load i32, ptr %aVar
  %b1 = load i32, ptr %bVar
  %sum1 = add i32 %a1, %b1
  %diff1 = sub i32 %sum1, 1
  %prod1 = mul i32 %diff1, %b1
  %quot1 = div i32 %prod1, 2
  store i32 %quot1, ptr %aVar
  %gep1 = getelementptr i32, i32* %aVar, i64 %i
  store i32 %quot1, ptr %gep1
  %readGep = load i32, ptr %gep1
  %cmpInner = icmp ne i32 %readGep, %secret
  br i1 %cmpInner, label %then_path, label %else_path
then_path:
  %thenAdd = add i32 %readGep, 3
  %thenMul = mul i32 %thenAdd, %secret
  store i32 %thenMul, ptr %bVar
  br label %merge_path
else_path:
  %elseSub = sub i32 %secret, %readGep
  %elseDiv = div i32 %elseSub, 2
  store i32 %elseDiv, ptr %bVar
  br label %merge_path
merge_path:
  %mergedVal = phi i32 [%thenMul, %then_path], [%elseDiv, %else_path]
  %nextI = add i64 %i, 1
  store i64 %nextI, ptr %idxVar
  %loopCond = icmp ne i64 %nextI, 4
  br i1 %loopCond, label %loop_header, label %after_loop
after_loop:
  %finalA = load i32, ptr %aVar
  %finalB = load i32, ptr %bVar
  %finalSum = add i32 %finalA, %finalB
  %finalCmp = icmp ne i32 %finalSum, 0
  br i1 %finalCmp, label %sink_secret, label %sink_public
sink_secret:
  call void @SINK(i32 %mergedVal)
  br label %return_block
sink_public:
  call void @SINK(i32 %finalSum)
  br label %return_block
early_exit:
  %earlyVal = load i32, ptr %cVar
  call void @SINK(i32 %earlyVal)
  br label %return_block
return_block:
  %retPhi = phi i32 [%mergedVal, %sink_secret], [%finalSum, %sink_public], [%earlyVal, %early_exit]
  ret i32 %retPhi
}

define i32 @branch_edges() #0 {
start:
  %xVar = alloca i32
  %yVar = alloca i32
  store i32 10, ptr %xVar
  store i32 20, ptr %yVar
  %x0 = load i32, ptr %xVar
  %y0 = load i32, ptr %yVar
  %first = icmp ne i32 %x0, %y0
  br i1 %first, label %left, label %right
left:
  %leftAdd = add i32 %x0, %y0
  %leftCmp = icmp ne i32 %leftAdd, 30
  br i1 %leftCmp, label %left_deep, label %join
left_deep:
  %deepMul = mul i32 %leftAdd, 2
  br label %join
right:
  %rightSub = sub i32 %y0, %x0
  br label %join
join:
  %joined = phi i32 [%deepMul, %left_deep], [%leftAdd, %left], [%rightSub, %right]
  %joinedCmp = icmp eq i32 %joined, 0
  br i1 %joinedCmp, label %zero_case, label %nonzero_case
zero_case:
  call void @SINK(i32 %joined)
  ret i32 0
nonzero_case:
  call void @SINK(i32 %joined)
  ret i32 %joined
}

define i32 @self_loop_and_calls() #0 {
begin:
  %counterVar = alloca i32
  %accVar = alloca i32
  %seed = call i32 (...) @SOURCE()
  store i32 0, ptr %counterVar
  store i32 %seed, ptr %accVar
  br label %spin
spin:
  %counter = load i32, ptr %counterVar
  %acc = load i32, ptr %accVar
  %accNext = add i32 %acc, %counter
  store i32 %accNext, ptr %accVar
  %counterNext = add i32 %counter, 1
  store i32 %counterNext, ptr %counterVar
  %keepGoing = icmp ne i32 %counterNext, 3
  br i1 %keepGoing, label %spin, label %done
done:
  %answer = load i32, ptr %accVar
  call void @SINK(i32 %answer)
  ret i32 %answer
}

define i32 @straight_line() #0 {
only:
  %plainVar = alloca i32
  store i32 7, ptr %plainVar
  %plainLoad = load i32, ptr %plainVar
  %plainAdd = add i32 %plainLoad, 5
  %plainSub = sub i32 %plainAdd, 2
  %plainMul = mul i32 %plainSub, 3
  %plainDiv = div i32 %plainMul, 2
  call void @SINK(i32 %plainDiv)
  ret i32 %plainDiv
}

declare i32 @SOURCE()
declare void @SINK(i32)
