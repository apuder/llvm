; RUN: llvm-as < %s | llvm-dis | FileCheck %s
; RUN: verify-uselistorder %s

; CHECK: !named = !{!0, !1, !1, !2, !2, !3, !3, !4}
!named = !{!0, !1, !2, !3, !4, !5, !6, !7}

!llvm.module.flags = !{!10}
!llvm.dbg.cu = !{!8}

; CHECK: !0 = distinct !DISubprogram(
!0 = distinct !DISubprogram()

; CHECK-NEXT: !1 = !DILocation(line: 3, column: 7, scope: !0)
!1 = !DILocation(line: 3, column: 7, scope: !0)
!2 = !DILocation(scope: !0, column: 7, line: 3)

; CHECK-NEXT: !2 = !DILocation(line: 3, column: 7, scope: !0, inlinedAt: !1)
!3 = !DILocation(scope: !0, inlinedAt: !1, column: 7, line: 3)
!4 = !DILocation(column: 7, line: 3, scope: !0, inlinedAt: !1)

; CHECK-NEXT: !3 = !DILocation(line: 0, scope: !0)
!5 = !DILocation(scope: !0)
!6 = !DILocation(scope: !0, column: 0, line: 0)

; CHECK-NEXT: !4 = !DILocation(line: 4294967295, column: 65535, scope: !0)
!7 = !DILocation(line: 4294967295, column: 65535, scope: !0)

!8 = distinct !DICompileUnit(language: DW_LANG_C99, producer: "clang",
                             file: !9,
                             isOptimized: true, flags: "-O2",
                             splitDebugFilename: "abc.debug", emissionKind: 2,
                             subprograms: !{!0})
!9 = !DIFile(filename: "path/to/file", directory: "/path/to/dir")
!10 = !{i32 2, !"Debug Info Version", i32 3}
