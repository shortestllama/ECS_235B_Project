define i32 @parser_core_ir(i32 %0, i32 %1, i1 %2) {
entry:
  %a = alloca i32, align 4
  store i32 %0, ptr %a, align 4
  %loaded = load i32, ptr %a, align 4
  %wide = sext i32 %loaded to i64
  %narrow = trunc i64 %wide to i32
  %mask = and i32 %narrow, 255
  %shift = shl i32 %mask, 1
  %choice = select i1 %2, i32 %shift, i32 %1
  switch i32 %choice, label %default [
    i32 0, label %zero
    i32 1, label %one
  ]

zero:
  %f0 = fadd double 1.000000e+00, 2.000000e+00
  %cmp0 = fcmp ogt double %f0, 0.000000e+00
  br label %done

one:
  %rem = srem i32 %choice, 2
  br label %done

default:
  %orv = or i32 %choice, 1
  br label %done

done:
  %out = phi i32 [ 0, %zero ], [ %rem, %one ], [ %orv, %default ]
  ret i32 %out
}
