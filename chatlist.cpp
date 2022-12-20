//
// Created by gsr on 2022/12/14.
//

#include "chatlist.h"

ChatInfo::ChatInfo()
{
    online_user = new list<User>;

    group_info = new list<Group>;

    //往group_info链表中添加群信息
    mydatabase = new ChatDataBase;
    mydatabase->my_database_connect("m_group");

    string group_name[MAXNUM];
    int group_num = mydatabase->my_database_get_group_name(group_name);

    for (int i = 0; i < group_num; ++i){
        Group g;
        g.name = group_name[i];
        g.l = new list<GroupUser>;    // 保存群中所有用户
        group_info->push_back(g);

        string member;              // 保存群里所有用户
        mydatabase->my_database_get_group_member(group_name[i], member);
        if (member.size() == 0)
        {
            continue;
        }

        // 解析组成员
        int start = 0, end = 0;
        GroupUser u;
        while (1)
        {
            end = member.find('|', start);
            if (-1 == end)
            {
                break;
            }
            u.name = member.substr(start, end - start);
            g.l->push_back(u);
            start = end + 1;
            u.name.clear();
        }
        u.name = member.substr(start, member.size() - start);
        g.l->push_back(u);
    }

    /*
    for (list<Group>::iterator it = group_info->begin(); it != group_info->end(); it++)
	{
		cout << "群名字 " << it->name << endl;
		for (list<GroupUser>::iterator i = it->l->begin(); i != it->l->end(); i++)
		{
			cout << i->name << endl;
		}
	}
    */

    mydatabase->my_database_disconnect();
    cout << "初始化链表成功" << endl;
}