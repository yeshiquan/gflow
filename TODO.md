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

#### 使用Thread Santinizer和Address Santinizer检查通过
    * BthreadExecutor无法通过Thread Santinizer的检查，应该是bthread自己的问题
    * AsyncExecutor可以通过Thread Santinizer的检查

#### 更友好的trace日志，可以打印vertex的名字 done

#### GraphData初始化时机的重构
    * 如果对所有声明的EMIT data，在process阶段统一初始化，那么没有显示赋值的data也会得到初始化。
      这意味着声明了EMIT的data，指针总是不为nullptr  done
    * process阶段的初始化要做成按需初始化，就是说如果Any已经分配空间了，就不要再分配一次空间了。 done
    * Any容器的对象怎么做回收，是否要区分T有无移动构造函数，有无构造函数, done, Any会负责析构对象
    * 多次执行graph，确保GraphData的空间不会释放，可以重复使用  done
       让同一个graph对象跑两个round，结果一样

#### expr支持bool类型 done

#### 增加DCHECK和likely、unlikely done 

#### 两个process对同一个data声明了不同的类型，要报错。done

#### 通过实现Commiter使得数据emit()后立即发布，从而执行的时候依赖更快被满足，图的驱动力更快

#### 实现Channel机制和MapReduce机制 done

#### Vertex Processor等对象使用ObjectPool创建

### 支持channel机制，实现Query容器，done

### 支持通用算子的定义 done