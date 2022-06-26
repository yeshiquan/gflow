## TODO
#### make_data变成setup一次性初始化 done

#### 预先build多个Graph实例, 配合objectpool，多线程运行 done

#### 支持group depend。 done

#### 图的执行 done
    * Closure模式，不支持两个vertex发布到同一个data，这样vertex全部执行完就意味着data也完成，简化判断。
    * 算子出错的情况
    * 是否要改成非递归的形式，现在的async是否有改进空间
    * 支持bthread或者自定义线程池执行图
    * 不支持on_finish的callback。

#### 支持Condition和条件表达式 done
    * data和condition的推导
    * 不支持三元操作表达式 a > b ? 2 : 1;
    * 条件依赖支持bool表达式 (a && !b || c >= 2) || (e != 5)
    * 不支持自定义发布新Data的表达式：
        * ExpressionProcessor::apply(builder, "A", "if (B > C) D+4 else E*5");

#### 使用boost pp简化图的使用 done

#### 支持vertex option done

#### 数据超过生命周期自动发布(Commiter) done

#### 根据名字来创建vertex，方便根据配置文件来创建图, context从processor移到vertex来保存。done

#### 两个process对同一个data声明了不同的类型，要报错。