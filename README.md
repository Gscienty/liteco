# Liteco 协程库

协程(coroutine)的概念由来已久，协程的定义如下:*协程是一个在多入口点允许在某些位置挂起和恢复执行的，可以产生非抢占式任务的子程序的计算机程序组件*。
协程与子程序的区别在于，主程序与子程序的关系是非对称的，是调用与被调用的关系，而协程则是完全对称的，它们可以相互调用。

## 简介

Liteco提供协程类`liteco_coroutine_t`，通过它可向您的系统提供协程能力。
