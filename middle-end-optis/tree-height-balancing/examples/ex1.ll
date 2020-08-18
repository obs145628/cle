; ModuleID = 'foo.c'
source_filename = "foo.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; Function Attrs: norecurse nounwind readonly uwtable
define dso_local i32 @foo(i32* nocapture readonly %0) local_unnamed_addr #0 {
  %a = load i32, i32* %0, align 4, !tbaa !2
  %bp = getelementptr inbounds i32, i32* %0, i64 1
  %b = load i32, i32* %bp, align 4, !tbaa !2
  %cp = getelementptr inbounds i32, i32* %0, i64 2
  %c = load i32, i32* %cp, align 4, !tbaa !2
  %dp = getelementptr inbounds i32, i32* %0, i64 3
  %d = load i32, i32* %dp, align 4, !tbaa !2
  %ep = getelementptr inbounds i32, i32* %0, i64 3
  %e = load i32, i32* %ep, align 4, !tbaa !2
  %fp = getelementptr inbounds i32, i32* %0, i64 3
  %f = load i32, i32* %fp, align 4, !tbaa !2
  %gp = getelementptr inbounds i32, i32* %0, i64 3
  %g = load i32, i32* %gp, align 4, !tbaa !2
  %hp = getelementptr inbounds i32, i32* %0, i64 3
  %h = load i32, i32* %hp, align 4, !tbaa !2
  
  %t1 = add nsw i32 %a, %b
  %t2 = add nsw i32 %t1, %c
  %t3 = add nsw i32 %t2, %d
  %t4 = add nsw i32 %t3, %e
  %t5 = add nsw i32 %t4, %f
  %t6 = add nsw i32 %t5, %g
  %t7 = add nsw i32 %t6, %h

  ret i32 %t7
}

attributes #0 = { norecurse nounwind readonly uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "frame-pointer"="none" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 10.0.0 (https://github.com/llvm/llvm-project.git d32170dbd5b0d54436537b6b75beaf44324e0c28)"}
!2 = !{!3, !3, i64 0}
!3 = !{!"int", !4, i64 0}
!4 = !{!"omnipotent char", !5, i64 0}
!5 = !{!"Simple C/C++ TBAA"}
