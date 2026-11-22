#pragma once

#include <string>

// категорія сповіщень - головна (батьківська) сутність
struct NotificationCategory
{
    int id = 0;               
    std::string name;       
    std::string description;  
    std::string channel;   
    int isActive = 1;        
};

// сповіщення - залежна (дочірня) сутність
struct Notification
{
    int id = 0;             
    int categoryId = 0;      
    std::string title;     
    int priority = 1;       
    std::string status;     
    int retryCount = 0;    
    std::string createdAt;   
};


// запис в історії змін статусу сповіщення 
struct NotificationStatusHistory
{
    int id = 0;
    int notificationId = 0;
    std::string oldStatus;    
    std::string newStatus;
    std::string changedAt;
};
