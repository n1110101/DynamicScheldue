#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <fstream>
#include <algorithm>

using namespace std;

//структура с необходимыми данными о задачах
struct Task
{
	string name; 
	int period; 
	int priority;
	int duration;
	Task(string s, int per, int pri, int d)
	{
		name = s;
		period = per;
		priority = pri;
		duration = d;
	}
};

//структура с запланированными задачами
struct Plan
{
	string name;
	int time;  //время поставки/продолжения
	string mode;  //start/continue
	Plan(Task task, int t, string m)
	{
		name = task.name;
		time = t;
		mode = m;
	}
};

//функция сравнения для сортировки задач по приоритету
bool compPriority(const Task& a, const Task& b)
{
	return a.priority > b.priority;
}

//функция сравнения для сортировки задач по времени
bool compTime(const Plan& a, const Plan& b)
{
	return a.time < b.time;
}

//проверяет задачи на корректность, удаляя некорректные
void deleteFails(vector<Task>& tasks)
{
	for(auto i = 0; i < int(tasks.size()); i++){
		int dur = tasks[i].duration;
		int per = tasks[i].period;
		if(per <= 0){
			cout<<"Error!Task "<<tasks[i].name<<" has incorrect period"<<endl;
			tasks.erase(tasks.begin() + i);
			continue;
		}
		if(dur > per || dur <= 0){
			cout<<"Error!Task "<<tasks[i].name<<" has incorrect duration"<<endl;
			tasks.erase(tasks.begin() + i);

		}
	}

}

//считываем данные из входного файла
void readTasks(const char* fname, vector<Task>& tasks, int& runtime)
{
	fstream fs;
	fs.open(fname, fstream::in);
	if(!fs){  //обработка ошибки чтения файла
		perror("open");
		exit(1);
	}
	string buf;
	getline(fs, buf);
	sscanf(buf.data(), "<system runtime=%*c%d%*c>", &runtime);
	while(1) {  //читаем из файла построчно, формируем вектор задач
		getline(fs, buf);
		if (buf[1] == '/')  //конец xml-таблицы
			break;
		int buf_size = buf.size();
		if(buf_size <= 1)  //пустая строка - пропуск итерации чтения
			continue;
		char name[buf_size];  //размер имени не больше длины входной строки
		int per, pri, dur;  //период, приоритет, длительность
		sscanf(buf.data(),
			"<task name=%s period=%*c%d%*c priority=%*c%d%*c duration=%*c%d%*c/>",
			name, &per,  &pri, &dur);
		tasks.push_back(Task(string(name), per, pri, dur));
	}
	fs.close();
}

//выводим результата планирования в файл
void writeSchedule(const vector<Plan>& plans, const int& runtime)
{
	fstream fs;
	fs.open("output.xml", fstream::out);
	if(!fs){  //обработка ошибки создания файла
		perror("open");
		exit(1);
	}
	fs << "<trace runtime=" <<'"'<<runtime<<'"'<< ">"<< endl;
	for (auto it : plans){	
		fs << "<" <<it.mode<<" name=" << it.name 
		<<" time=" << '"' << it.time << '"'<< "/>" << endl;
	}
	fs << "</trace>" << endl;
	fs.close();
}

//проверка на наличие свободного времени для задачи
bool checkFreeTime(const Task& task, const vector<int>& timeline, const int& i)
{
		int runtime = timeline.size();
		int j = i*task.period;  //сдвиг согласно текущему периоду
		int test_size = task.duration;
		while(test_size && (j < (i+1)*task.period) && (j < runtime)){
			if(timeline[j] == -1){  //найдено свободное время
				test_size--;
			}
			j++;
		}
		if(test_size != 0){  //недостаточно свободного времени в периоде
			return false;
		}	
		return true;
}

//вычсляем расписание 
vector<int> computeSchedule(vector<Plan>& plans,
			const vector<Task>& tasks, const int& runtime)
{
	vector<int> timeline (runtime, -1);  //вектор, сопоставляющий шкале времени индекс задачи
	for(auto it = 0 ; it < tasks.size(); it++){
		int percount = runtime / tasks[it].period;  //количество периодов задачи
		if(runtime % tasks[it].period != 0)
			percount++;
		for(auto i = 0; i < percount; i++){  //цикл по периодам
			Task task = tasks[it];
			if(checkFreeTime(task, timeline, i) == false){
				cout<<"Error!Cannot put task "<<task.name
				<<" in its "<<i<<" period "<<endl; 
				continue;  //проппускаем период из-за нехватки места для задачи
			}
			int j = i*task.period; //позиция в текущем периоде
			int size = task.duration;
			bool disrupt = false;  //преванность задачи в периоде
			bool start = false;  //начало постановки задачи в периоде
			while(size && (j < (i+1)*task.period) && (j < runtime)){
				if(timeline[j] == -1){
					if(!start){
						start = true;  //задача поставлена
						plans.push_back(Plan(task, j, "start"));
					}
					if(disrupt && start){
						disrupt = false;  //продолжение прерванной задачи	
						plans.push_back(Plan(task, j, "continue"));
					}
					timeline[j] = it;  //занимаем шкалу времени задачей
					size--;  //задаче осталось size времени выполнения
				}
				else if(start){
					disrupt = true;  //задача прервана
				}
				j++;
			}
		}
	}
	return timeline;
}

//программа получает имя файла в качестве входных данных
int main(int argc, char** argv)
{
	if (argc != 2){ //обработка ошибки командной строки	
		cout<<"Wrong commandline parameters, expecting .xml file"<<endl;
		return 0;
	}

	vector<Task> tasks;
	vector<Plan> plans;
	int runtime;

	readTasks(argv[1], tasks, runtime);  //чтение данных о задачах
	deleteFails(tasks);  //проверяем задачи на корректность 
	stable_sort(tasks.begin(), tasks.end(), compPriority);  //cортировка задач по приоритету

	computeSchedule(plans, tasks, runtime);  //вычиcление  запланированных задач 

	sort(plans.begin(), plans.end(), compTime);  //сортировка вектора запланированных задач по времени
	writeSchedule(plans, runtime);
	return 0;
}