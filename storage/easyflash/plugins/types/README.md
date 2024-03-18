# EasyFlash Types �����plugin��

---

## 1������

Ŀǰ EasyFlash �Ὣ�����������ַ�����ʽ�洢�� Flash �У�������ģʽ�£����ڷ��ַ������͵Ļ���������ʹ��ʱ���ͱ�������Ӷ�����ַ���ת�����롣��� Types �������Ϊ�˷����û���ʹ�� EasyFlash ʱ���Ը��Ӽ򵥵ķ�ʽȥ�����������͵Ļ���������

��Ҫ֧�ֵ����Ͱ�����C �� **��������** �� **��������** �Լ� **�ṹ������** �����ڽṹ�����ͣ� Types ����ڲ����� [struct2json](https://github.com/armink/struct2json) �����ת����������Ŀ����Ҫ���� [struct2json](https://github.com/armink/struct2json) �⡣

## 2��ʹ��

### 2.1 Դ�뵼��

����֮ǰ��Ҫȷ���Լ�����Ŀ���Ѱ��� EasyFlash ����Դ�룬������ "\easyflash\inc"��"\easyflash\port" �� "\easyflash\src" ������ļ������뷽�����Բο������ֲ�ĵ���[�����](https://github.com/armink/EasyFlash/blob/master/docs/zh/port.md)�����ٽ� Types ���Դ�뵼�뵽��Ŀ�У������ͬ "plugins\types" �ļ���һ�𿽱�����Ŀ�����е� easyflash �ļ����¡�Ȼ����Ҫ��� `easyflash\plugins\types\struct2json\inc` �� `easyflash\plugins\types` �����ļ���·������Ŀͷ�ļ�·���м��ɡ�

### 2.2 ��ʼ��

```C
void ef_types_init(S2jHook *hook)
```

�������Ϊ Types ����ĳ�ʼ����������Ҫ��ʼ�� struct2json ��������ڴ��������Ĭ��ʹ�õ� malloc �� free ��Ϊ�ڴ�����������ʹ��Ĭ���ڴ����ʽ���������ʼ�����������ʹ�� RT-Thread ����ϵͳ�Դ����ڴ������������Բο�����ĳ�ʼ�����룺

```C
S2jHook s2jHook = {
        .free_fn = rt_free,
        .malloc_fn = (void *(*)(size_t))rt_malloc,
};
ef_types_init(&s2jHook);
```

### 2.3 ������������

#### 2.3.1 ��������

���ڻ������͵Ļ����������������� EasyFlash ԭ�е� API һ�£�ֻ���޸�����μ����ε����ͣ����п��õ� API ���£�

```C
bool ef_get_bool(const char *key);
char ef_get_char(const char *key);
short ef_get_short(const char *key);
int ef_get_int(const char *key);
long ef_get_long(const char *key);
float ef_get_float(const char *key);
double ef_get_double(const char *key);
EfErrCode ef_set_bool(const char *key, bool value);
EfErrCode ef_set_char(const char *key, char value);
EfErrCode ef_set_short(const char *key, short value);
EfErrCode ef_set_int(const char *key, int value);
EfErrCode ef_set_long(const char *key, long value);
EfErrCode ef_set_float(const char *key, float value);
EfErrCode ef_set_double(const char *key, double value);
```

#### 2.3.2 ��������

��������͵Ĳ�����������һ�£���ͬ�����ڣ���ȡ���Ļ���������ͨ��ָ�����͵���ν��з��ء����п��õ� API ���£�

```C
void ef_get_bool_array(const char *key, bool *value);
void ef_get_char_array(const char *key, char *value);
void ef_get_short_array(const char *key, short *value);
void ef_get_int_array(const char *key, int *value);
void ef_get_long_array(const char *key, long *value);
void ef_get_float_array(const char *key, float *value);
void ef_get_double_array(const char *key, double *value);
void ef_get_string_array(const char *key, char **value);
EfErrCode ef_set_bool_array(const char *key, bool *value, size_t len);
EfErrCode ef_set_char_array(const char *key, char *value, size_t len);
EfErrCode ef_set_short_array(const char *key, short *value, size_t len);
EfErrCode ef_set_int_array(const char *key, int *value, size_t len);
EfErrCode ef_set_long_array(const char *key, long *value, size_t len);
EfErrCode ef_set_float_array(const char *key, float *value, size_t len);
EfErrCode ef_set_double_array(const char *key, double *value, size_t len);
EfErrCode ef_set_string_array(const char *key, char **value, size_t len);
```

#### 2.3.3 �ṹ������

���ڽṹ�����ͣ�����������Ҫʹ�� struct2json ������д��ýṹ���Ӧ�� JSON ��ת�������ٽ���д�õĻ�ת������Ϊ��ν���ʹ�á��ṹ�����ͻ������������� API ���£�

```C
void *ef_get_struct(const char *key, ef_types_get_cb get_cb);
EfErrCode ef_set_struct(const char *key, void *value, ef_types_set_cb set_cb);
```

����ʹ�����̼��ṹ���� JSON ֮��Ļ�ת�������Բο������ Demo��

```C
/* ����ṹ�� */
typedef struct {
    char name[16];
} Hometown;
typedef struct {
    uint8_t id;
    double weight;
    uint8_t score[8];
    char name[16];
    Hometown hometown;
} Student;

/* ����ṹ��ת JSON �ķ��� */
static cJSON *stu_set_cb(void* struct_obj) {
    Student *struct_student = (Student *)struct_obj;
    /* ���� Student JSON ���� */
    s2j_create_json_obj(json_student);
    /* ���л����ݵ� Student JSON ���� */
    s2j_json_set_basic_element(json_student, struct_student, int, id);
    s2j_json_set_basic_element(json_student, struct_student, double, weight);
    s2j_json_set_array_element(json_student, struct_student, int, score, 8);
    s2j_json_set_basic_element(json_student, struct_student, string, name);
    /* ���л����ݵ� Student.Hometown JSON ���� */
    s2j_json_set_struct_element(json_hometown, json_student, struct_hometown, struct_student, Hometown, hometown);
    s2j_json_set_basic_element(json_hometown, struct_hometown, string, name);
    return json_student;
}

/* ���� JSON ת�ṹ��ķ��� */
static void *stu_get_cb(cJSON* json_obj) {
    /* ���� Student �ṹ�������ʾ�� s2j_ ��ͷ�ķ����� struct2json ���ṩ�ģ� */
    s2j_create_struct_obj(struct_student, Student);
    /* �����л����ݵ� Student �ṹ����� */
    s2j_struct_get_basic_element(struct_student, json_obj, int, id);
    s2j_struct_get_array_element(struct_student, json_obj, int, score);
    s2j_struct_get_basic_element(struct_student, json_obj, string, name);
    s2j_struct_get_basic_element(struct_student, json_obj, double, weight);
    /* �����л����ݵ� Student.Hometown �ṹ����� */
    s2j_struct_get_struct_element(struct_hometown, struct_student, json_hometown, json_obj, Hometown, hometown);
    s2j_struct_get_basic_element(struct_hometown, json_hometown, string, name);
    return struct_student;
}

/* ���ýṹ�����ͻ������� */
Student orignal_student = {
        .id = 24,
        .weight = 71.2,
        .score = {1, 2, 3, 4, 5, 6, 7, 8},
        .name = "����",
        .hometown.name = "����",
};
ef_set_struct("����ѧ��", &orignal_student, stu_set_cb);

/* ��ȡ�ṹ�����ͻ������� */
Student *student;
student = ef_get_struct("����ѧ��", stu_get_cb);

/* ��ӡ��ȡ���Ľṹ������ */
printf("������%s ���᣺%s \n", student->name, student->hometown.name);

/* �ͷŻ�ȡ�ṹ�����ͻ������������п��ٵĶ�̬�ڴ� */
s2jHook.free_fn(student);
```
