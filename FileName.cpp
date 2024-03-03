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

// 方便起见给命名空间取别名
namespace fs = std::filesystem;

//创建日志文件
std::ofstream logoutput("myLOG.txt");

int file_num;
int dic_num;
int de;
int longest_path_length;
fs::path longest_path_name;

struct TreeNode {
    fs::path filePath;
    bool isDirectory;
    std::vector<TreeNode*> children; // 孩子节点
    std::time_t lastWriteTime;
    long long filesize;
    TreeNode* sibling; // 兄弟节点
    TreeNode* parent; // 父母节点
};

//兄弟孩子树结构如下
struct an_TreeNode {
    fs::path filePath;
    bool isDirectory;
    std::time_t lastWriteTime; // 添加 lastWriteTime 成员
    long long filesize; // 添加 filesize 成员
    an_TreeNode* children; // 孩子节点
    an_TreeNode* sibling; // 兄弟节点

    an_TreeNode(const fs::path& path, bool isDir, std::time_t writeTime, long long size) : filePath(path), isDirectory(isDir), lastWriteTime(writeTime), filesize(size), children(nullptr), sibling(nullptr) {}

};

//扫描时用于暂时存储文件信息
struct FileInfo {
    fs::path filePath; // fs::path 类型的成员变量，用于存储文件路径
    int integerValue;   // int 类型的成员变量，用于存储整数值
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

//扫描C盘
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

//验证使用者是否有管理员权限，保证结果一致
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

//打印树，检查用
void printTree(TreeNode* node, int depth) {
    if (node == nullptr) {
        return;
    }

    // 打印当前节点信息
    for (int i = 0; i < depth; ++i) {
        std::cout << "  "; // 缩进以表示深度
    }
    if (node->isDirectory) {
        std::cout << "[Directory] ";
    }
    else {
        std::cout << "[File] ";
    }
    std::cout << node->filePath << std::endl;

    // 递归打印子节点
    for (TreeNode* child : node->children) {
        printTree(child, depth + 1);
    }
}

an_TreeNode* convertTreeNode(TreeNode* node) {
    if (node == nullptr)
        return nullptr;

    //将普通的树转为孩子兄弟树方便接下来的各项操作
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

//删除中间树所用空间
void deleteTreeNode(TreeNode* node) {
    if (node == nullptr)
        return;

    for (TreeNode* child : node->children) {
        deleteTreeNode(child); // 递归释放子节点的空间
    }
    delete node; // 释放当前节点的空间
}

//打印树，检查用
void printNewTree(an_TreeNode* node, int depth) {
    if (node == nullptr) {
        return;
    }

    // 打印当前节点信息
    for (int i = 0; i < depth; ++i) {
        std::cout << "  "; // 缩进以表示深度
    }
    if (node->isDirectory) {
        std::cout << "[Directory] ";
    }
    else {
        std::cout << "[File] ";
    }
    std::cout << node->filePath << std::endl;

    // 递归打印子节点
    printNewTree(node->children, depth + 1);
    printNewTree(node->sibling, depth);
}

//用深度优先算法获得树深
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

        // 添加孩子节点
        if (currentNode->children != nullptr) {
            nodesQueue.push({ currentNode->children, depth + 1 });

            // 添加兄弟节点
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

//寻找指定节点
an_TreeNode* findNodeByPath(an_TreeNode* currentNode, const fs::path& targetPath) {
    std::stack<an_TreeNode*> nodesStack;
    nodesStack.push(currentNode);

    while (!nodesStack.empty()) {
        an_TreeNode* node = nodesStack.top();
        nodesStack.pop();

        // 检查当前节点是否匹配目标路径
        if (node->filePath == targetPath) {
            return node; // 如果匹配，则返回当前节点
        }

        // 将兄弟节点压入栈中（先压入右兄弟节点，再压入左兄弟节点，保证左兄弟节点先被处理）
        if (node->sibling != nullptr) {
            nodesStack.push(node->sibling);
        }

        // 将子节点压入栈中（保证子节点先被处理）
        if (node->children != nullptr) {
            nodesStack.push(node->children);
        }
    }

    // 如果未找到目标节点，则返回空指针
    return nullptr;
}

//删除指定节点
void deleteNodeByPath(an_TreeNode* currentNode, const fs::path& targetPath) {
    std::stack<an_TreeNode*> nodesStack;
    nodesStack.push(currentNode);

    while (!nodesStack.empty()) {
        an_TreeNode* node = nodesStack.top();
        nodesStack.pop();

        // 检查当前节点是否匹配目标路径
        if (node->filePath == targetPath) {
            // 修改当前节点为nullptr
            node->filePath.clear(); // 清除文件路径
            node->isDirectory = 1;
            node->children = nullptr; // 清空子节点
            std::cout << "删除成功" << std::endl;
            std::cout << "----------------------------------" << std::endl;
            return; // 如果匹配，则返回当前节点
        }

        // 将兄弟节点压入栈中（先压入右兄弟节点，再压入左兄弟节点，保证左兄弟节点先被处理）
        if (node->sibling != nullptr) {
            nodesStack.push(node->sibling);
        }

        // 将子节点压入栈中（保证子节点先被处理）
        if (node->children != nullptr) {
            nodesStack.push(node->children);
        }
    }
    std::cout << "删除失败" << std::endl;
    std::cout << "----------------------------------" << std::endl;
    // 如果未找到目标节点，则返回空指针
    return;
}

//对比“mystart.txt”文件中出现的文件夹内容
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
                std::cout << "文件个数差异：" << fileCount - other_fileCount << std::endl;
                di_num++;
                logoutput << "^^^" << userInput << ",文件个数差异：" << fileCount - other_fileCount << std::endl;
            }if (totalSize != other_totalSize) {
                std::cout << "Directory are different:" << userInput << std::endl;
                std::cout << "文件总大小差异：" << totalSize - other_totalSize << std::endl;
                di_num++;
                logoutput << "^^^" << userInput << ",文件总大小差异：" << totalSize - other_totalSize << std::endl;
            }if (earliestFile->filePath != other_earliestFile->filePath ||
                earliestFile->filesize != other_earliestFile->filesize ||
                earliestFile->lastWriteTime != other_earliestFile->lastWriteTime) {
                std::cout << "Directory are different:" << userInput << std::endl;
                std::cout << "最早时间文件：" << other_earliestFile->filePath << std::endl;
                std::cout << other_earliestFile->filesize << "字节,File time:" << other_earliestFile->lastWriteTime << std::endl;
                di_num++;
                logoutput << "^^^" << userInput << ",最早时间文件：" << other_earliestFile->filePath << ",大小" << other_earliestFile->filesize << "字节,File time:" << other_earliestFile->lastWriteTime << std::endl;
            }if (latestFile->filePath != other_latestFile->filePath ||
                latestFile->filesize != other_latestFile->filesize ||
                latestFile->lastWriteTime != other_latestFile->lastWriteTime) {
                std::cout << "Directory are different:" << userInput << std::endl;
                std::cout << "最晚时间文件：" << other_latestFile->filePath << std::endl;
                std::cout << other_latestFile->filesize << "字节,File time:" << other_latestFile->lastWriteTime << std::endl;
                di_num++;
                logoutput << "^^^" << userInput << ",最早时间文件：" << other_latestFile->filePath << ",大小" << other_latestFile->filesize << "字节,File time:" << other_latestFile->lastWriteTime << std::endl;
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
            logoutput << "^^^" << userInput << ",文件个数差异：" << fileCount << std::endl;
            logoutput << "^^^" << userInput << ",文件总大小差异：" << totalSize << std::endl;
            logoutput << "^^^" << userInput << ",最早时间文件：" << earliestFile->filePath << ",大小" << earliestFile->filesize << "字节,File time:" << other_earliestFile->lastWriteTime << std::endl;
            logoutput << "^^^" << userInput << ",最早时间文件：" << latestFile->filePath << ",大小" << latestFile->filesize << "字节,File time:" << other_latestFile->lastWriteTime << std::endl;
        }
        else {
            std::cout << "Node not found." << userInput << std::endl;
        }
    }
    std::cout << "共检查目录" << dirs1.size() << "个,";
    std::cout << "存在差异" << di_num << "个。" << std::endl;
    std::cout << "----------------------------------" << std::endl;
    logoutput << "--- 共比较" << dirs1.size() << "个目录，有" << di_num << "个差异！" << std::endl;
}

//分割字符串
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
    // 找到最后一个 '\' 的位置
    size_t lastBackslashPos = targetPath.find_last_of('\\');

    // 如果找到了最后一个 '\'，则删除其后的所有字符
    if (lastBackslashPos != std::string::npos) {
        targetPath.erase(lastBackslashPos);
    }
}

//处理“myfile.txt”
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

    // 对dirs1中的每个字符串根据“，”进行分割
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
                std::cout << "file:" << splitDirs1[i] << std::endl << "添加成功" << std::endl;
                std::cout << "----------------------------------" << std::endl;
            }
            else {
                std::cout << "----------------------------------" << std::endl;
                std::cout << "file:" << splitDirs1[i] << std::endl << "添加失败，上级文件夹不存在" << std::endl;
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
                std::cout << "file:" << splitDirs1[i] << std::endl << "修改成功" << std::endl;
                std::cout << "----------------------------------" << std::endl;
            }
            else {
                std::cout << "----------------------------------" << std::endl;
                std::cout << "file:" << splitDirs1[i] << std::endl << "修改失败，文件不存在" << std::endl;
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

//处理“mydir.txt”
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

    // 对dirs1中的每个字符串根据“，”进行分割
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

//文件操作后选择三个文件进行对比，说明修改成功
void test_file(const std::string& filePath, an_TreeNode* newRoot, an_TreeNode* other_root) {
    fs::path targetPath(filePath);
    std::cout << "改变测试：" << targetPath << std::endl;
    an_TreeNode* foundNode = findNodeByPath(newRoot, targetPath);
    an_TreeNode* other_foundNode = findNodeByPath(other_root, targetPath);
    logoutput << std::endl;
    logoutput << "改变测试：" << targetPath << std::endl;
    if (foundNode != nullptr) {
        std::cout << "变化前: 大小 " << foundNode->filesize << ",时间 " << foundNode->lastWriteTime << std::endl;
        logoutput << "变化前: 大小 " << foundNode->filesize << ",时间 " << foundNode->lastWriteTime << std::endl;
    }
    else {
        std::cout << "变化前文件不存在" << std::endl;
        logoutput << "变化前文件不存在" << std::endl;
    }
    if (other_foundNode != nullptr) {
        std::cout << "变化后: 大小 " << other_foundNode->filesize << ",时间 " << other_foundNode->lastWriteTime << std::endl;
        logoutput << "变化后: 大小 " << other_foundNode->filesize << ",时间 " << other_foundNode->lastWriteTime << std::endl;
    }
    else {
        std::cout << "变化后文件不存在" << std::endl;
        logoutput << "变化后文件不存在" << std::endl;
    }
    std::cout << std::endl;
}

int main()
{
    // 检查程序是否以管理员身份运行
    if (!IsRunAsAdministrator())
    {
        std::cerr << "Please run the program as an administrator." << std::endl;
        return 1;
    }

    // 打开输出文件以进行写入
    std::ofstream outputFile("output.txt");


    outputFile << "Drop database cscan;" << std::endl;
    outputFile << "CREATE DATABASE IF NOT EXISTS cscan;" << std::endl;
    outputFile << "USE cscan;" << std::endl;
    outputFile << "CREATE TABLE IF NOT EXISTS FileInfo (" << std::endl;
    outputFile << "id INT AUTO_INCREMENT PRIMARY KEY, filename VARCHAR(255),filepath VARCHAR(1023),filesize VARCHAR(21),last_modified VARCHAR(21),depth INT); " << std::endl;
    outputFile << "CREATE TABLE IF NOT EXISTS DicInfo (" << std::endl;
    outputFile << "id INT AUTO_INCREMENT PRIMARY KEY, filename VARCHAR(255),filepath VARCHAR(1023),filesize VARCHAR(21),last_modified VARCHAR(21),depth INT); " << std::endl;
    outputFile << "" << std::endl;

    // 检查输出文件是否成功打开
    if (!outputFile.is_open()) {
        std::cerr << "Failed to open output file." << std::endl;
        return 1;
    }

    file_num = 0;// 文件数量
    std::string rootPath = "C://Windows";//扫描所选根目录
    logoutput << "目录：c:\\windows\\" << std::endl;
    fs::path rootDir(rootPath);

    TreeNode* root = new TreeNode(); // 分配内存并初始化根节点
    root->filePath = rootPath; // 设置根节点的文件路径
    root->isDirectory = true; // 根节点是一个目录
    root->parent = nullptr; // 根节点没有父节点;

    FileInfo rootInfo;
    rootInfo.filePath = rootDir;
    rootInfo.integerValue = 1;
    rootInfo.node = root;
    pathQueue.push(rootInfo); // 将根目录信息添加到队列中

    //使用队列非递归扫描指定目录
    while (!pathQueue.empty()) {
        try {//使用try，防止因为权限问题中途推出
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

    // 新建一个名为 other_root 的根节点
    an_TreeNode* other_root = convertTreeNode(root);

    deleteTreeNode(root);

    int treeDepth = getTreeDepth(newRoot);
    std::cout << "depth of tree：" << treeDepth << std::endl;
    logoutput << "depth of tree：" << treeDepth << std::endl;

    outputFile << "CREATE TABLE FileStatistics (id INT AUTO_INCREMENT PRIMARY KEY,file_number INT,directory_number INT,max_depth INT,Longest_file_path VARCHAR(255),Length_of_longest_file_path INT,tree_depth INT); " << std::endl;
    outputFile << "INSERT INTO FileStatistics (file_number,directory_number,max_depth,Longest_file_path,Length_of_longest_file_path,tree_depth)VALUES(";
    outputFile << file_num << "," << dic_num << "," << de << "," << longest_path_name << "," << longest_path_length << "," << treeDepth << ");" << std::endl;
    outputFile.close();

    std::cout << "----------------------------------" << std::endl;

    while (1) {
        int chs_option = 0;
        //功能选择
        std::cout << "请输入功能选择：" << std::endl;
        std::cout << "1.统计指定目录中的文件信息" << std::endl;
        std::cout << "2.模拟操作" << std::endl;
        std::cout << "3.退出" << std::endl;
        std::cin >> chs_option;
        std::cout << "----------------------------------" << std::endl;

        if (chs_option == 1) {
            std::cout << "请输入目标路径：" << std::endl;

            // 声明变量以保存用户输入的路径
            std::string userInput;
            std::cin >> userInput;

            // 将用户输入的路径转换为 fs::path 类型
            fs::path targetPath(userInput);

            an_TreeNode* this_node = new an_TreeNode("null", 1, 0, 0);
            int fileCount = 0;
            long long totalSize = 0;
            an_TreeNode* earliestFile = this_node;
            long long ear_time = 9223372036854775807;//初始最早时间设为无穷大
            an_TreeNode* latestFile = this_node;
            long long las_time = 0;//初始最晚时间设为0

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
            std::cout << "展示3条操作后的文件信息变化:" << std::endl;
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
            std::cout << "成功退出" << std::endl;
            std::cout << "----------------------------------" << std::endl;
            return 0;
        }
        else {
            std::cout << "----------------------------------" << std::endl;
            std::cout << "输入无效，请重试" << std::endl;
            std::cout << "----------------------------------" << std::endl;
        }
    }


    logoutput.close();
    return 0;
}
