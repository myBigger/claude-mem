#ifndef TEMPLATE_H
#define TEMPLATE_H
#include <QString>
struct Template {
    int id=0; QString templateId, name, recordType;
    int version=1; QString content, status;
    int createdBy=0; QString createdAt, updatedAt;
};
#endif
