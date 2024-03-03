#include <iostream>
#include <string>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <Windows.h>
#include <algorithm>
#include <queue>
#include <stack>
#include <unordered_map>

// ��������������ռ�ȡ����
namespace fs = std::filesystem;

//������־�ļ�
std::ofstream logoutput("myLOG.txt");

int file_num;
int dic_num;
int de;
int longest_path_length;
fs::path longest_path_name;

struct TreeNode {
    fs::path filePath;
    bool isDirectory;
    std::vector<TreeNode*> children; // ���ӽڵ�
    std::time_t lastWriteTime;
    long long filesize;
    TreeNode* sibling; // �ֵܽڵ�
    TreeNode* parent; // ��ĸ�ڵ�
};

//�ֵܺ������ṹ����
struct an_TreeNode {
    fs::path filePath;
    bool isDirectory;
    std::time_t lastWriteTime; // ��� lastWriteTime ��Ա
    long long filesize; // ��� filesize ��Ա
    an_TreeNode* children; // ���ӽڵ�
    an_TreeNode* sibling; // �ֵܽڵ�

    an_TreeNode(const fs::path& path, bool isDir, std::time_t writeTime, long long size) : filePath(path), isDirectory(isDir), lastWriteTime(writeTime), filesize(size), children(nullptr), sibling(nullptr) {}

};

//ɨ��ʱ������ʱ�洢�ļ���Ϣ
struct FileInfo {
    fs::path filePath; // fs::path ���͵ĳ�Ա���������ڴ洢�ļ�·��
    int integerValue;   // int ���͵ĳ�Ա���������ڴ洢����ֵ
    TreeNode* node;
};

//std::queue<fs::path> pathQueue;
std::queue<FileInfo> pathQueue;

std::time_t getLastWriteTime(const fs::path& filePath) {
    try {
        auto lastWriteTime = fs::last_write_time(filePath);
        auto timePoint = std::chrono::time_point_cast<std::chrono::system_clock::duration>(lastWriteTime - fs::file_time_type::clock::now() + std::chrono::system_clock::now());
        return std::chrono::system_clock::to_time_t(timePoint);
    }
    catch (const std::filesystem::filesystem_error& ex) {
        //std::cerr << "Filesystem Error: " << ex.what() << std::endl;
        return std::time_t{};
    }
}

//ɨ��C��
void processDirectory(const fs::path& dir, int a, std::ofstream& outputFile, TreeNode* t_node)
{
    de = max(de, a);
    for (const auto& entry : fs::directory_iterator(dir))
    {
        try {
            if (fs::is_directory(entry.path()))
            {
                dic_num++;
                std::time_t lastWriteTime = getLastWriteTime(entry.path());
                if (lastWriteTime != std::time_t{}) {
                    //outputFile << "Last write time: " << lastWriteTime << std::endl;
                }
                outputFile << "INSERT INTO DicInfo (filename, filepath, filesize, last_modified, depth)VALUES(";
                outputFile << entry.path().filename() << "," << entry.path() << "," << fs::file_size(entry.path()) << "," << lastWriteTime << "," << a + 1 << ");" << std::endl;
                outputFile << " " << std::endl;
                FileInfo reInfo;
                reInfo.filePath = entry.path();
                reInfo.integerValue = a + 1;
                reInfo.node = new TreeNode();
                reInfo.node->parent = t_node;
                reInfo.node->filesize = fs::file_size(entry.path());
                reInfo.node->lastWriteTime = lastWriteTime;
                reInfo.node->filePath = entry.path();
                reInfo.node->isDirectory = true;
                t_node->children.push_back(reInfo.node);
                pathQueue.push(reInfo);
            }
            else if (fs::is_regular_file(entry.path()))
            {
                file_num++;
                std::time_t lastWriteTime = getLastWriteTime(entry.path());
                outputFile << "INSERT INTO FileInfo (filename, filepath, filesize, last_modified, depth)VALUES(";
                outputFile << entry.path().filename() << "," << entry.path() << "," << fs::file_size(entry.path()) << "," << lastWriteTime << "," << a << ");" << std::endl;
                outputFile << " " << std::endl;

                struct TreeNode* if_node = new TreeNode();
                if_node->filePath = entry.path();
                if_node->isDirectory = false;
                if_node->filesize = fs::file_size(entry.path());
                if_node->lastWriteTime = lastWriteTime;
                if_node->parent = t_node;
                t_node->children.push_back(if_node);

                if (entry.path().wstring().length() > longest_path_length) {
                    longest_path_length = entry.path().wstring().length();
                    longest_path_name = entry.path().wstring();
                }
            }
        }
        catch (const std::filesystem::filesystem_error& ex) {
            //std::cerr << "Error: " << ex.what() << std::endl;
        }
    }
}

//��֤ʹ�����Ƿ��й���ԱȨ�ޣ���֤���һ��
bool IsRunAsAdministrator()
{
    BOOL isAdmin = FALSE;
    PSID administratorsGroup;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    if (AllocateAndInitializeSid(&NtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &administratorsGroup))
    {
        CheckTokenMembership(NULL, administratorsGroup, &isAdmin);
        FreeSid(administratorsGroup);
    }
    return isAdmin != FALSE;
}

//��ӡ���������
void printTree(TreeNode* node, int depth) {
    if (node == nullptr) {
        return;
    }

    // ��ӡ��ǰ�ڵ���Ϣ
    for (int i = 0; i < depth; ++i) {
        std::cout << "  "; // �����Ա�ʾ���
    }
    if (node->isDirectory) {
        std::cout << "[Directory] ";
    }
    else {
        std::cout << "[File] ";
    }
    std::cout << node->filePath << std::endl;

    // �ݹ��ӡ�ӽڵ�
    for (TreeNode* child : node->children) {
        printTree(child, depth + 1);
    }
}

an_TreeNode* convertTreeNode(TreeNode* node) {
    if (node == nullptr)
        return nullptr;

    //����ͨ����תΪ�����ֵ�������������ĸ������
    an_TreeNode* newNode = new an_TreeNode(node->filePath, node->isDirectory, node->lastWriteTime, node->filesize);
    if (node->children.size() > 0) {
        newNode->children = convertTreeNode(node->children[0]);
        an_TreeNode* sibling = newNode->children;
        for (size_t i = 1; i < node->children.size(); ++i) {
            sibling->sibling = convertTreeNode(node->children[i]);
            sibling = sibling->sibling;
        }
    }
    return newNode;
}

//ɾ���м������ÿռ�
void deleteTreeNode(TreeNode* node) {
    if (node == nullptr)
        return;

    for (TreeNode* child : node->children) {
        deleteTreeNode(child); // �ݹ��ͷ��ӽڵ�Ŀռ�
    }
    delete node; // �ͷŵ�ǰ�ڵ�Ŀռ�
}

//��ӡ���������
void printNewTree(an_TreeNode* node, int depth) {
    if (node == nullptr) {
        return;
    }

    // ��ӡ��ǰ�ڵ���Ϣ
    for (int i = 0; i < depth; ++i) {
        std::cout << "  "; // �����Ա�ʾ���
    }
    if (node->isDirectory) {
        std::cout << "[Directory] ";
    }
    else {
        std::cout << "[File] ";
    }
    std::cout << node->filePath << std::endl;

    // �ݹ��ӡ�ӽڵ�
    printNewTree(node->children, depth + 1);
    printNewTree(node->sibling, depth);
}

//����������㷨�������
int getTreeDepth(an_TreeNode* root) {
    if (root == nullptr)
        return 0;

    std::queue<std::pair<an_TreeNode*, int>> nodesQueue;
    nodesQueue.push({ root, 1 });
    int maxDepth = 0;

    while (!nodesQueue.empty()) {
        auto [currentNode, depth] = nodesQueue.front();
        nodesQueue.pop();
        maxDepth = max(maxDepth, depth);

        // ��Ӻ��ӽڵ�
        if (currentNode->children != nullptr) {
            nodesQueue.push({ currentNode->children, depth + 1 });

            // ����ֵܽڵ�
            auto siblingNode = currentNode->children->sibling;
            int siblingDepth = depth;
            while (siblingNode != nullptr) {
                nodesQueue.push({ siblingNode, siblingDepth });
                siblingNode = siblingNode->sibling;
                siblingDepth++;
            }
        }
    }

    return maxDepth;
}

//Ѱ��ָ���ڵ�
an_TreeNode* findNodeByPath(an_TreeNode* currentNode, const fs::path& targetPath) {
    std::stack<an_TreeNode*> nodesStack;
    nodesStack.push(currentNode);

    while (!nodesStack.empty()) {
        an_TreeNode* node = nodesStack.top();
        nodesStack.pop();

        // ��鵱ǰ�ڵ��Ƿ�ƥ��Ŀ��·��
        if (node->filePath == targetPath) {
            return node; // ���ƥ�䣬�򷵻ص�ǰ�ڵ�
        }

        // ���ֵܽڵ�ѹ��ջ�У���ѹ�����ֵܽڵ㣬��ѹ�����ֵܽڵ㣬��֤���ֵܽڵ��ȱ�����
        if (node->sibling != nullptr) {
            nodesStack.push(node->sibling);
        }

        // ���ӽڵ�ѹ��ջ�У���֤�ӽڵ��ȱ�����
        if (node->children != nullptr) {
            nodesStack.push(node->children);
        }
    }

    // ���δ�ҵ�Ŀ��ڵ㣬�򷵻ؿ�ָ��
    return nullptr;
}

//ɾ��ָ���ڵ�
void deleteNodeByPath(an_TreeNode* currentNode, const fs::path& targetPath) {
    std::stack<an_TreeNode*> nodesStack;
    nodesStack.push(currentNode);

    while (!nodesStack.empty()) {
        an_TreeNode* node = nodesStack.top();
        nodesStack.pop();

        // ��鵱ǰ�ڵ��Ƿ�ƥ��Ŀ��·��
        if (node->filePath == targetPath) {
            // �޸ĵ�ǰ�ڵ�Ϊnullptr
            node->filePath.clear(); // ����ļ�·��
            node->isDirectory = 1;
            node->children = nullptr; // ����ӽڵ�
            std::cout << "ɾ���ɹ�" << std::endl;
            std::cout << "----------------------------------" << std::endl;
            return; // ���ƥ�䣬�򷵻ص�ǰ�ڵ�
        }

        // ���ֵܽڵ�ѹ��ջ�У���ѹ�����ֵܽڵ㣬��ѹ�����ֵܽڵ㣬��֤���ֵܽڵ��ȱ�����
        if (node->sibling != nullptr) {
            nodesStack.push(node->sibling);
        }

        // ���ӽڵ�ѹ��ջ�У���֤�ӽڵ��ȱ�����
        if (node->children != nullptr) {
            nodesStack.push(node->children);
        }
    }
    std::cout << "ɾ��ʧ��" << std::endl;
    std::cout << "----------------------------------" << std::endl;
    // ���δ�ҵ�Ŀ��ڵ㣬�򷵻ؿ�ָ��
    return;
}

//�Աȡ�mystart.txt���ļ��г��ֵ��ļ�������
void compareDirectories(const std::string& filePath, an_TreeNode* newRoot, an_TreeNode* other_root) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filePath << std::endl;
        return;
    }

    std::string line;
    bool startComparing = false;
    std::vector<std::string> dirs1;

    while (std::getline(file, line)) {
        if (line == "stat dirs") {
            startComparing = true;
            continue;
        }
        else if (line == "end of dirs") {
            break;
        }

        if (startComparing) {
            dirs1.push_back(line);
        }
    }

    file.close();

    int di_num = 0;

    std::cout << "----------------------------------" << std::endl;
    for (std::string& userInput : dirs1) {
        if (!userInput.empty()) {
            // Remove the last character from the line
            userInput.pop_back();
        }
        fs::path targetPath(userInput);

        an_TreeNode* this_node = new an_TreeNode("null", 1, 0, 0);;
        int fileCount = 0;
        long long totalSize = 0;
        an_TreeNode* earliestFile = this_node;
        long long ear_time = 9223372036854775807;
        an_TreeNode* latestFile = this_node;
        long long las_time = 0;

        an_TreeNode* other_node = new an_TreeNode("null", 1, 0, 0);;
        int other_fileCount = 0;
        long long other_totalSize = 0;
        an_TreeNode* other_earliestFile = other_node;
        long long other_ear_time = 9223372036854775807;
        an_TreeNode* other_latestFile = other_node;
        long long other_las_time = 0;

        an_TreeNode* foundNode = findNodeByPath(newRoot, targetPath);
        an_TreeNode* other_foundNode = findNodeByPath(other_root, targetPath); //->filePath.empty()
        if (foundNode != nullptr && other_foundNode != nullptr) {
            this_node = foundNode->children;
            while (this_node != nullptr) {
                if (this_node->isDirectory == 1) {
                    this_node = this_node->sibling;
                    continue;
                }
                fileCount++;
                totalSize += this_node->filesize;

                if (this_node->lastWriteTime < ear_time) {
                    earliestFile = this_node;
                    ear_time = this_node->lastWriteTime;
                }

                if (this_node->lastWriteTime > las_time) {
                    latestFile = this_node;
                    las_time = this_node->lastWriteTime;
                }

                this_node = this_node->sibling;
            }

            other_node = other_foundNode->children;
            while (other_node != nullptr) {
                if (other_node->isDirectory == 1) {
                    other_node = other_node->sibling;
                    continue;
                }
                other_fileCount++;
                other_totalSize += other_node->filesize;

                if (other_node->lastWriteTime < other_ear_time) {
                    other_earliestFile = other_node;
                    other_ear_time = other_node->lastWriteTime;
                }

                if (other_node->lastWriteTime > other_las_time) {
                    other_latestFile = other_node;
                    other_las_time = other_node->lastWriteTime;
                }

                other_node = other_node->sibling;
            }

            if (fileCount != other_fileCount) {
                std::cout << "Directory are different:" << userInput << std::endl;
                std::cout << "�ļ��������죺" << fileCount - other_fileCount << std::endl;
                di_num++;
                logoutput << "^^^" << userInput << ",�ļ��������죺" << fileCount - other_fileCount << std::endl;
            }if (totalSize != other_totalSize) {
                std::cout << "Directory are different:" << userInput << std::endl;
                std::cout << "�ļ��ܴ�С���죺" << totalSize - other_totalSize << std::endl;
                di_num++;
                logoutput << "^^^" << userInput << ",�ļ��ܴ�С���죺" << totalSize - other_totalSize << std::endl;
            }if (earliestFile->filePath != other_earliestFile->filePath ||
                earliestFile->filesize != other_earliestFile->filesize ||
                earliestFile->lastWriteTime != other_earliestFile->lastWriteTime) {
                std::cout << "Directory are different:" << userInput << std::endl;
                std::cout << "����ʱ���ļ���" << other_earliestFile->filePath << std::endl;
                std::cout << other_earliestFile->filesize << "�ֽ�,File time:" << other_earliestFile->lastWriteTime << std::endl;
                di_num++;
                logoutput << "^^^" << userInput << ",����ʱ���ļ���" << other_earliestFile->filePath << ",��С" << other_earliestFile->filesize << "�ֽ�,File time:" << other_earliestFile->lastWriteTime << std::endl;
            }if (latestFile->filePath != other_latestFile->filePath ||
                latestFile->filesize != other_latestFile->filesize ||
                latestFile->lastWriteTime != other_latestFile->lastWriteTime) {
                std::cout << "Directory are different:" << userInput << std::endl;
                std::cout << "����ʱ���ļ���" << other_latestFile->filePath << std::endl;
                std::cout << other_latestFile->filesize << "�ֽ�,File time:" << other_latestFile->lastWriteTime << std::endl;
                di_num++;
                logoutput << "^^^" << userInput << ",����ʱ���ļ���" << other_latestFile->filePath << ",��С" << other_latestFile->filesize << "�ֽ�,File time:" << other_latestFile->lastWriteTime << std::endl;
            }
        }
        else if (foundNode != nullptr && other_foundNode == nullptr) {
            std::cout << "Node has already deleted:" << userInput << std::endl;
            std::cout << "File count in directory: " << fileCount << std::endl;
            std::cout << "Total size of files in directory: " << totalSize << " bytes" << std::endl;
            std::cout << "Earliest file: " << earliestFile->filePath.filename() << std::endl;
            std::cout << "Earliest file size: " << earliestFile->filesize << std::endl;
            std::cout << "Earliest file last_motified time: " << earliestFile->lastWriteTime << std::endl;
            std::cout << "Latest file: " << latestFile->filePath.filename() << std::endl;
            std::cout << "Earliest file size: " << latestFile->filesize << std::endl;
            std::cout << "Earliest file last_motified time: " << latestFile->lastWriteTime << std::endl;
            di_num += 4;
            logoutput << "^^^" << userInput << ",�ļ��������죺" << fileCount << std::endl;
            logoutput << "^^^" << userInput << ",�ļ��ܴ�С���죺" << totalSize << std::endl;
            logoutput << "^^^" << userInput << ",����ʱ���ļ���" << earliestFile->filePath << ",��С" << earliestFile->filesize << "�ֽ�,File time:" << other_earliestFile->lastWriteTime << std::endl;
            logoutput << "^^^" << userInput << ",����ʱ���ļ���" << latestFile->filePath << ",��С" << latestFile->filesize << "�ֽ�,File time:" << other_latestFile->lastWriteTime << std::endl;
        }
        else {
            std::cout << "Node not found." << userInput << std::endl;
        }
    }
    std::cout << "�����Ŀ¼" << dirs1.size() << "��,";
    std::cout << "���ڲ���" << di_num << "����" << std::endl;
    std::cout << "----------------------------------" << std::endl;
    logoutput << "--- ���Ƚ�" << dirs1.size() << "��Ŀ¼����" << di_num << "�����죡" << std::endl;
}

//�ָ��ַ���
std::vector<std::string> splitString(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    while (std::getline(ss, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

void removeLastPathComponent(std::string& targetPath) {
    // �ҵ����һ�� '\' ��λ��
    size_t lastBackslashPos = targetPath.find_last_of('\\');

    // ����ҵ������һ�� '\'����ɾ�����������ַ�
    if (lastBackslashPos != std::string::npos) {
        targetPath.erase(lastBackslashPos);
    }
}

//����myfile.txt��
void deal_myfile(const std::string& filePath, an_TreeNode* newRoot, an_TreeNode* other_root) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filePath << std::endl;
        return;
    }

    std::string line;
    bool startComparing = false;
    std::vector<std::string> dirs;

    while (std::getline(file, line)) {
        if (line == "selected files") {
            startComparing = true;
            continue;
        }
        else if (line == "end of files") {
            break;
        }

        if (startComparing) {
            dirs.push_back(line);
        }
    }

    file.close();

    // ��dirs1�е�ÿ���ַ������ݡ��������зָ�
    std::vector<std::string> splitDirs1;
    for (const std::string& dir : dirs) {
        std::vector<std::string> tokens = splitString(dir, ',');
        splitDirs1.insert(splitDirs1.end(), tokens.begin(), tokens.end());
    }

    for (size_t i = 0; i < splitDirs1.size(); i += 4) {
        if (splitDirs1[i + 1] == "A") {
            long long new_size = stoll(splitDirs1[i + 3]);
            std::time_t new_time = static_cast<std::time_t>(std::stoll(splitDirs1[i + 2]));
            std::string path_str = splitDirs1[i];
            removeLastPathComponent(path_str);
            fs::path targetPath(path_str);
            an_TreeNode* foundNode = findNodeByPath(other_root, targetPath);
            an_TreeNode* A_file = new an_TreeNode(splitDirs1[i], 0, new_time, new_size);
            if (foundNode != nullptr) {
                A_file->sibling = foundNode->children;
                foundNode->children = A_file;
                std::cout << "----------------------------------" << std::endl;
                std::cout << "file:" << splitDirs1[i] << std::endl << "��ӳɹ�" << std::endl;
                std::cout << "----------------------------------" << std::endl;
            }
            else {
                std::cout << "----------------------------------" << std::endl;
                std::cout << "file:" << splitDirs1[i] << std::endl << "���ʧ�ܣ��ϼ��ļ��в�����" << std::endl;
                std::cout << "----------------------------------" << std::endl;
            }

        }
        // category M
        else if (splitDirs1[i + 1] == "M") {
            fs::path targetPath(splitDirs1[i]);
            long long new_size = stoll(splitDirs1[i + 3]);
            std::time_t new_time = static_cast<std::time_t>(std::stoll(splitDirs1[i + 2]));
            an_TreeNode* foundNode = findNodeByPath(newRoot, targetPath);
            if (foundNode != nullptr) {
                foundNode->filePath = targetPath;
                foundNode->filesize = new_size;
                foundNode->lastWriteTime = new_time;
                std::cout << "----------------------------------" << std::endl;
                std::cout << "file:" << splitDirs1[i] << std::endl << "�޸ĳɹ�" << std::endl;
                std::cout << "----------------------------------" << std::endl;
            }
            else {
                std::cout << "----------------------------------" << std::endl;
                std::cout << "file:" << splitDirs1[i] << std::endl << "�޸�ʧ�ܣ��ļ�������" << std::endl;
                std::cout << "----------------------------------" << std::endl;
            }
        }
        // category D
        else if (splitDirs1[i + 1] == "D") {
            fs::path targetPath(splitDirs1[i]);
            std::cout << "----------------------------------" << std::endl;
            std::cout << "file:" << splitDirs1[i] << std::endl;
            deleteNodeByPath(other_root, targetPath);
        }
    }
}

std::string splitStringAndGetFirst(const std::string& str, char delimiter) {
    std::stringstream ss(str);
    std::string token;
    std::getline(ss, token, delimiter);
    return token;
}

//����mydir.txt��
void deal_mydir(const std::string& filePath, an_TreeNode* newRoot, an_TreeNode* other_root) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filePath << std::endl;
        return;
    }

    std::string line;
    bool startComparing = false;
    std::vector<std::string> dirs;

    while (std::getline(file, line)) {
        if (line == "selected dirs") {
            startComparing = true;
            continue;
        }
        else if (line == "end of dirs") {
            break;
        }

        if (startComparing) {
            dirs.push_back(line);
        }
    }

    file.close();

    // ��dirs1�е�ÿ���ַ������ݡ��������зָ�
    std::vector<std::string> splitDirs;
    for (const std::string& dir : dirs) {
        std::string firstToken = splitStringAndGetFirst(dir, ',');
        splitDirs.push_back(firstToken);
    }

    for (std::string& userInput : splitDirs) {
        if (!userInput.empty()) {
            // Remove the last character from the line
            userInput.pop_back();
        }
        fs::path targetPath(userInput);

        an_TreeNode* this_node = new an_TreeNode("null", 1, 0, 0);
        int fileCount = 0;
        long long totalSize = 0;
        an_TreeNode* earliestFile = this_node;
        long long ear_time = 9223372036854775807;
        an_TreeNode* latestFile = this_node;
        long long las_time = 0;

        an_TreeNode* foundNode = findNodeByPath(other_root, targetPath);
        if (foundNode != nullptr) {
            this_node = foundNode->children;
            while (this_node != nullptr) {
                if (this_node->isDirectory == 1) {
                    this_node = this_node->sibling;
                    continue;
                }
                fileCount++;
                totalSize += this_node->filesize;

                if (this_node->lastWriteTime < ear_time) {
                    earliestFile = this_node;
                    ear_time = this_node->lastWriteTime;
                }

                if (this_node->lastWriteTime > las_time) {
                    latestFile = this_node;
                    las_time = this_node->lastWriteTime;
                }

                this_node = this_node->sibling;
            }
            std::cout << "----------------------------------" << std::endl;
            std::cout << "Name of directory: " << userInput << std::endl;
            std::cout << "File count in directory: " << fileCount << std::endl;
            std::cout << "Total size of files in directory: " << totalSize << " bytes" << std::endl;
            std::cout << "Earliest file: " << earliestFile->filePath.filename() << std::endl;
            std::cout << "Earliest file size: " << earliestFile->filesize << std::endl;
            std::cout << "Earliest file last_motified time: " << earliestFile->lastWriteTime << std::endl;
            std::cout << "Latest file: " << latestFile->filePath.filename() << std::endl;
            std::cout << "Earliest file size: " << latestFile->filesize << std::endl;
            std::cout << "Earliest file last_motified time: " << latestFile->lastWriteTime << std::endl;
            deleteNodeByPath(other_root, targetPath);
        }
        else {
            std::cout << "----------------------------------" << std::endl;
            std::cout << "Name of directory: " << userInput << std::endl;
            std::cout << "Node not found." << std::endl;
            std::cout << "----------------------------------" << std::endl;
        }
    }
}

//�ļ�������ѡ�������ļ����жԱȣ�˵���޸ĳɹ�
void test_file(const std::string& filePath, an_TreeNode* newRoot, an_TreeNode* other_root) {
    fs::path targetPath(filePath);
    std::cout << "�ı���ԣ�" << targetPath << std::endl;
    an_TreeNode* foundNode = findNodeByPath(newRoot, targetPath);
    an_TreeNode* other_foundNode = findNodeByPath(other_root, targetPath);
    logoutput << std::endl;
    logoutput << "�ı���ԣ�" << targetPath << std::endl;
    if (foundNode != nullptr) {
        std::cout << "�仯ǰ: ��С " << foundNode->filesize << ",ʱ�� " << foundNode->lastWriteTime << std::endl;
        logoutput << "�仯ǰ: ��С " << foundNode->filesize << ",ʱ�� " << foundNode->lastWriteTime << std::endl;
    }
    else {
        std::cout << "�仯ǰ�ļ�������" << std::endl;
        logoutput << "�仯ǰ�ļ�������" << std::endl;
    }
    if (other_foundNode != nullptr) {
        std::cout << "�仯��: ��С " << other_foundNode->filesize << ",ʱ�� " << other_foundNode->lastWriteTime << std::endl;
        logoutput << "�仯��: ��С " << other_foundNode->filesize << ",ʱ�� " << other_foundNode->lastWriteTime << std::endl;
    }
    else {
        std::cout << "�仯���ļ�������" << std::endl;
        logoutput << "�仯���ļ�������" << std::endl;
    }
    std::cout << std::endl;
}

int main()
{
    // �������Ƿ��Թ���Ա�������
    if (!IsRunAsAdministrator())
    {
        std::cerr << "Please run the program as an administrator." << std::endl;
        return 1;
    }

    // ������ļ��Խ���д��
    std::ofstream outputFile("output.txt");


    outputFile << "Drop database cscan;" << std::endl;
    outputFile << "CREATE DATABASE IF NOT EXISTS cscan;" << std::endl;
    outputFile << "USE cscan;" << std::endl;
    outputFile << "CREATE TABLE IF NOT EXISTS FileInfo (" << std::endl;
    outputFile << "id INT AUTO_INCREMENT PRIMARY KEY, filename VARCHAR(255),filepath VARCHAR(1023),filesize VARCHAR(21),last_modified VARCHAR(21),depth INT); " << std::endl;
    outputFile << "CREATE TABLE IF NOT EXISTS DicInfo (" << std::endl;
    outputFile << "id INT AUTO_INCREMENT PRIMARY KEY, filename VARCHAR(255),filepath VARCHAR(1023),filesize VARCHAR(21),last_modified VARCHAR(21),depth INT); " << std::endl;
    outputFile << "" << std::endl;

    // �������ļ��Ƿ�ɹ���
    if (!outputFile.is_open()) {
        std::cerr << "Failed to open output file." << std::endl;
        return 1;
    }

    file_num = 0;// �ļ�����
    std::string rootPath = "C://Windows";//ɨ����ѡ��Ŀ¼
    logoutput << "Ŀ¼��c:\\windows\\" << std::endl;
    fs::path rootDir(rootPath);

    TreeNode* root = new TreeNode(); // �����ڴ沢��ʼ�����ڵ�
    root->filePath = rootPath; // ���ø��ڵ���ļ�·��
    root->isDirectory = true; // ���ڵ���һ��Ŀ¼
    root->parent = nullptr; // ���ڵ�û�и��ڵ�;

    FileInfo rootInfo;
    rootInfo.filePath = rootDir;
    rootInfo.integerValue = 1;
    rootInfo.node = root;
    pathQueue.push(rootInfo); // ����Ŀ¼��Ϣ��ӵ�������

    //ʹ�ö��зǵݹ�ɨ��ָ��Ŀ¼
    while (!pathQueue.empty()) {
        try {//ʹ��try����ֹ��ΪȨ��������;�Ƴ�
            processDirectory(pathQueue.front().filePath, pathQueue.front().integerValue, outputFile, pathQueue.front().node);
        }
        catch (const std::exception& ex) {
            //std::cerr << "Error: " << ex.what() << std::endl;
        }
        pathQueue.pop();
    }

    std::cout << "File number:" << file_num << std::endl;
    std::cout << "dictionary number:" << dic_num << std::endl;
    std::cout << "Max depth:" << de << std::endl;
    std::cout << "Longest file path: " << longest_path_name << std::endl;
    std::cout << "Length of longest file path: " << longest_path_length << std::endl;

    logoutput << "File number:" << file_num << std::endl;
    logoutput << "dictionary number:" << dic_num << std::endl;
    logoutput << "Max depth:" << de << std::endl;
    logoutput << "Longest file path: " << longest_path_name << std::endl;
    logoutput << "Length of longest file path: " << longest_path_length << std::endl;

    an_TreeNode* newRoot = convertTreeNode(root);

    // �½�һ����Ϊ other_root �ĸ��ڵ�
    an_TreeNode* other_root = convertTreeNode(root);

    deleteTreeNode(root);

    int treeDepth = getTreeDepth(newRoot);
    std::cout << "depth of tree��" << treeDepth << std::endl;
    logoutput << "depth of tree��" << treeDepth << std::endl;

    outputFile << "CREATE TABLE FileStatistics (id INT AUTO_INCREMENT PRIMARY KEY,file_number INT,directory_number INT,max_depth INT,Longest_file_path VARCHAR(255),Length_of_longest_file_path INT,tree_depth INT); " << std::endl;
    outputFile << "INSERT INTO FileStatistics (file_number,directory_number,max_depth,Longest_file_path,Length_of_longest_file_path,tree_depth)VALUES(";
    outputFile << file_num << "," << dic_num << "," << de << "," << longest_path_name << "," << longest_path_length << "," << treeDepth << ");" << std::endl;
    outputFile.close();

    std::cout << "----------------------------------" << std::endl;

    while (1) {
        int chs_option = 0;
        //����ѡ��
        std::cout << "�����빦��ѡ��" << std::endl;
        std::cout << "1.ͳ��ָ��Ŀ¼�е��ļ���Ϣ" << std::endl;
        std::cout << "2.ģ�����" << std::endl;
        std::cout << "3.�˳�" << std::endl;
        std::cin >> chs_option;
        std::cout << "----------------------------------" << std::endl;

        if (chs_option == 1) {
            std::cout << "������Ŀ��·����" << std::endl;

            // ���������Ա����û������·��
            std::string userInput;
            std::cin >> userInput;

            // ���û������·��ת��Ϊ fs::path ����
            fs::path targetPath(userInput);

            an_TreeNode* this_node = new an_TreeNode("null", 1, 0, 0);
            int fileCount = 0;
            long long totalSize = 0;
            an_TreeNode* earliestFile = this_node;
            long long ear_time = 9223372036854775807;//��ʼ����ʱ����Ϊ�����
            an_TreeNode* latestFile = this_node;
            long long las_time = 0;//��ʼ����ʱ����Ϊ0

            an_TreeNode* foundNode = findNodeByPath(newRoot, targetPath);
            if (foundNode != nullptr) {
                this_node = foundNode->children;
                while (this_node != nullptr) {
                    if (this_node->isDirectory == 1) {
                        this_node = this_node->sibling;
                        continue;
                    }
                    fileCount++;
                    totalSize += this_node->filesize;

                    if (this_node->lastWriteTime < ear_time) {
                        earliestFile = this_node;
                        ear_time = this_node->lastWriteTime;
                    }

                    if (this_node->lastWriteTime > las_time) {
                        latestFile = this_node;
                        las_time = this_node->lastWriteTime;
                    }

                    this_node = this_node->sibling;
                }

                std::cout << "----------------------------------" << std::endl;
                std::cout << "File count in directory: " << fileCount << std::endl;
                std::cout << "Total size of files in directory: " << totalSize << " bytes" << std::endl;
                std::cout << "Earliest file: " << earliestFile->filePath.filename() << std::endl;
                std::cout << "Earliest file size: " << earliestFile->filesize << std::endl;
                std::cout << "Earliest file last_motified time: " << earliestFile->lastWriteTime << std::endl;
                std::cout << "Latest file: " << latestFile->filePath.filename() << std::endl;
                std::cout << "Earliest file size: " << latestFile->filesize << std::endl;
                std::cout << "Earliest file last_motified time: " << latestFile->lastWriteTime << std::endl;
                std::cout << "----------------------------------" << std::endl;
            }
            else {
                std::cout << "----------------------------------" << std::endl;
                std::cout << "Node not found." << std::endl;
                std::cout << "----------------------------------" << std::endl;
            }
        }
        else if (chs_option == 2) {
            std::string mystat = "mystat.txt";

            std::string myfile = "myfile.txt";
            deal_myfile(myfile, newRoot, other_root);
            std::cout << "----------------------------------" << std::endl;
            std::cout << "չʾ3����������ļ���Ϣ�仯:" << std::endl;
            std::string myfile1 = "C://Windows/System32/wbem/zh-CN/MDMAppProv.dll.mui2401";
            std::string myfile2 = "C://Windows/SysWOW64/zh-CN/DevDispItemProvider.dll.mui";
            std::string myfile3 = "C://Windows/Fonts/ega80737.fon";
            test_file(myfile1, newRoot, other_root);
            test_file(myfile2, newRoot, other_root);
            test_file(myfile3, newRoot, other_root);
            std::cout << "----------------------------------" << std::endl;
            compareDirectories(mystat, newRoot, other_root);


            std::string mydir = "mydir.txt";
            deal_mydir(mydir, newRoot, other_root);
            compareDirectories(mystat, newRoot, other_root);
        }
        else if (chs_option == 3) {
            std::cout << "----------------------------------" << std::endl;
            std::cout << "�ɹ��˳�" << std::endl;
            std::cout << "----------------------------------" << std::endl;
            return 0;
        }
        else {
            std::cout << "----------------------------------" << std::endl;
            std::cout << "������Ч��������" << std::endl;
            std::cout << "----------------------------------" << std::endl;
        }
    }


    logoutput.close();
    return 0;
}
