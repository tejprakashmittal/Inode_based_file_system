#include<bits/stdc++.h>
#include<unistd.h>
#define TOTAL_BLOCKS 128000
#define BLOCK_SIZE 4096
#define NO_OF_DATA_BLOCKS 99000
#define NO_OF_INODES 28000
#define INODE_START_INDEX 1000
#define DATA_BLOCK_START_INDEX 29000
using namespace std;

string current_disk;
FILE *disk_pointer;

unordered_map<string,int> fName_to_fd;    //key--filename val--fd
unordered_map<int,int> open_files;  //fd to mode(0-read,1-write,2-append)
string file_modes[3] = {"Read","Write","Append"};

struct iNode{   // total size 8 bytes
    int fileSize; // 4 bytes
    int dataBlock;  //4bytes
};

struct fileName_To_iNode_map{ //total size 24 bytes
    char fileName[20]; //max 20 bytes
    int iNode_index; // 4 bytes   inode number == inode index == fileDescriptor
};

struct superBlock{  //64 +16 = 80 B
    int i_bitmap[NO_OF_INODES];    //8 B
    int d_bitmap[NO_OF_DATA_BLOCKS]; // 8 B
    struct fileName_To_iNode_map fnameToiNodeMap[NO_OF_INODES]; // 24*2=48 B
    struct iNode iNode_array[NO_OF_INODES]; //2*8=16 B
}Super_Block;

int create_disk(string diskname){
    struct superBlock Super_Block;
    char buffer[BLOCK_SIZE];
    bzero(buffer,BLOCK_SIZE);
    disk_pointer = fopen(diskname.c_str(),"wb");
    for(int i=0;i<TOTAL_BLOCKS;i++){
        fwrite(buffer,sizeof(char),BLOCK_SIZE,disk_pointer);
    }
    for(int i=0;i<NO_OF_INODES;i++){
        Super_Block.i_bitmap[i] = 1;
    }
    for(int i=0;i<NO_OF_DATA_BLOCKS;i++){
        Super_Block.d_bitmap[i] = 1;
    }
    for(int i=0;i<NO_OF_INODES;i++){
        Super_Block.fnameToiNodeMap[i].fileName[20]=0;
        Super_Block.fnameToiNodeMap[i].iNode_index=-1;
        Super_Block.iNode_array[i].fileSize=-1;
        Super_Block.iNode_array[i].dataBlock=-1;
    }
    char s_block_buffer[sizeof(Super_Block)];
    bzero(s_block_buffer,sizeof(Super_Block));
    memcpy(s_block_buffer,&Super_Block,sizeof(Super_Block));
    fseek(disk_pointer, 0, SEEK_SET);
    fwrite(s_block_buffer,sizeof(char),sizeof(Super_Block),disk_pointer);
    fclose(disk_pointer);
    return 1;
}

int mount_disk(string diskname){
   disk_pointer = fopen(diskname.c_str(),"rb+"); 
   fseek(disk_pointer,0,SEEK_SET);
   char buffer[sizeof(Super_Block)];
   bzero(buffer,sizeof(Super_Block));
   fread(buffer,sizeof(char),sizeof(Super_Block),disk_pointer);
   memcpy(&Super_Block,buffer,sizeof(Super_Block));
   for(int i=0;i<NO_OF_INODES;i++){
       if(Super_Block.i_bitmap[i] == 0){
          fName_to_fd[Super_Block.fnameToiNodeMap[i].fileName] = Super_Block.fnameToiNodeMap[i].iNode_index;
       }
   }
   return 1;
}

int unmount_disk(string diskname){
   //disk_pointer = fopen(diskname.c_str(),"rb+"); 
   char buffer[sizeof(Super_Block)];
   bzero(buffer,sizeof(Super_Block));
   bzero(buffer,sizeof(Super_Block));
   memcpy(buffer,&Super_Block,sizeof(Super_Block));
   fseek(disk_pointer,0,SEEK_SET);
   fwrite(buffer,sizeof(char),sizeof(Super_Block),disk_pointer);
   fName_to_fd.clear();
   open_files.clear();
   fclose(disk_pointer);
}

int create_file(string filename){
    //FILE *disk_pointer = fopen(current_disk.c_str(),"rb"); 
    int i_index=-1,d_index=-1; // both contains relative indices not the actual ones
    for(int i=0;i<NO_OF_INODES;i++){
        if(Super_Block.i_bitmap[i]==1){
            i_index = i;
            break;
        }
    }
    for(int i=0;i<NO_OF_DATA_BLOCKS;i++){
        if(Super_Block.d_bitmap[i] == 1){
            d_index = i;
            break;
        }
    }
    if(i_index == -1 || d_index == -1){
        cout<<"inode_index "<<i_index<<endl<<"datablock "<<d_index<<endl;
        return -1;
    }
    Super_Block.iNode_array[i_index].dataBlock = d_index + DATA_BLOCK_START_INDEX; // actual datablock index
    Super_Block.i_bitmap[i_index]=0;
    strcpy(Super_Block.fnameToiNodeMap[i_index].fileName,filename.c_str());
    Super_Block.fnameToiNodeMap[i_index].iNode_index = i_index;
    Super_Block.d_bitmap[d_index]=0;
    fName_to_fd[filename] = i_index;
    //fclose(disk_pointer);
    return i_index;// + INODE_START_INDEX;  //actual index
}

int write_file(int fd){
    int db_index = Super_Block.iNode_array[fd].dataBlock;
    if(db_index > 0)
        fseek(disk_pointer,db_index*BLOCK_SIZE,SEEK_SET);
    else
        return -1;
    cout<<"Type the text to be written: "<<endl;
    int read_count=0;
    char input_buff[BLOCK_SIZE];
    bzero(input_buff,BLOCK_SIZE);
    // int i=0;
    // char c='\0';
    // while((c=getchar())!='\n'){
    //     input_buff[i++] = c;
    // }
    read_count = read(STDIN_FILENO,input_buff,BLOCK_SIZE);
    fwrite(input_buff,sizeof(char),read_count-1,disk_pointer);
    Super_Block.iNode_array[fd].fileSize = read_count-1;
    return 1;
}

int read_file(int fd){
    int db_index = Super_Block.iNode_array[fd].dataBlock;
    if(db_index > 0)
        fseek(disk_pointer,db_index*BLOCK_SIZE,SEEK_SET);
    else
        return -1;
    char buff[BLOCK_SIZE];
    bzero(buff,BLOCK_SIZE);
    fread(buff,sizeof(char),BLOCK_SIZE,disk_pointer);
    cout<<buff<<endl;
    fflush(stdout);
    return 1;
}

int append_file(int fd){
    int db_index = Super_Block.iNode_array[fd].dataBlock;
    int filesize = Super_Block.iNode_array[fd].fileSize;
    if(db_index > 0)
            fseek(disk_pointer,db_index*BLOCK_SIZE+filesize,SEEK_SET);
    else
            return -1; 
    cout<<"Type the text to be appended: "<<endl;
    int read_count=0;
    char input_buff[BLOCK_SIZE];
    bzero(input_buff,BLOCK_SIZE);
    // int i=0;
    // char c='\0';
    // while((c=getchar())!='\n'){
    //     input_buff[i++] = c;
    // }
    read_count = read(STDIN_FILENO,input_buff,BLOCK_SIZE);
    fwrite(input_buff,sizeof(char),read_count-1,disk_pointer);
    Super_Block.iNode_array[fd].fileSize = filesize + read_count-1;
    return 1;
}

int open_file(string filename,int mode){
    if(fName_to_fd.find(filename) == fName_to_fd.end()){
        cout<<"File is not created."<<endl;
        return -1;
    }
    int fd = fName_to_fd[filename];
    open_files[fd] = mode;
    return fd;
}

bool isOpen(int fd){
    if(open_files.find(fd)!=open_files.end()) return true;
    return false;
}

int delete_file(string filename){
    int fd = fName_to_fd[filename];
    if(isOpen(fd)){
        open_files.erase(fd);
    }
    fName_to_fd.erase(filename);
    Super_Block.i_bitmap[fd] = 1;
    strcpy(Super_Block.fnameToiNodeMap[fd].fileName,"");
    int d_block_index = Super_Block.iNode_array[fd].dataBlock - DATA_BLOCK_START_INDEX;
    Super_Block.d_bitmap[d_block_index] = 1;
    Super_Block.iNode_array[fd].dataBlock = -1;
    Super_Block.iNode_array[fd].fileSize = -1;
    return 1;
}

int main(){
    int i;
    while(1){
        cout<<"1: create disk"<<endl;
        cout<<"2: mount disk"<<endl;
        cout<<"3: exit"<<endl;
        cin>>i;
        if(i==1){
            string diskname;
            cout<<"Enter a unique disk name:"<<endl;
            cin>>diskname;
            create_disk(diskname);
        }
        else if(i==2){
            cout<<"Enter the disk name:"<<endl;
            string diskname;
            cin>>diskname;
            if(mount_disk(diskname) == 1){
                current_disk = diskname;
                while(1)
                {
                    cout<<"1: create file"<<endl;
                    cout<<"2: open file"<<endl;
                    cout<<"3: read file"<<endl;
                    cout<<"4: append file"<<endl;
                    cout<<"5: write file"<<endl;
                    cout<<"6: close file"<<endl;
                    cout<<"7: delete file"<<endl;
                    cout<<"8: list of files"<<endl;
                    cout<<"9: list of opened files"<<endl;
                    cout<<"10: unmount disk"<<endl;
                    int j;
                    cin>>j;
                    if(j==1){
                        string filename;
                        cout<<"Enter a unique file name:"<<endl;
                        cin>>filename;
                        int fd = create_file(filename);
                        cout<<"File create with descriptor: "<<fd<<endl;
                    }
                    else if(j==2){
                        string filename;
                        cout<<"Enter the file name:"<<endl;
                        cin>>filename;
                        int mode;
                        cout<<"0: read mode"<<endl;
                        cout<<"1: write mode"<<endl;
                        cout<<"2: append mode"<<endl;
                        cin>>mode;
                        int fd = open_file(filename,mode);
                        cout<<"File opened with descriptor: "<<fd<<endl;
                    }
                    else if(j==3){
                        int fd;
                        cout<<"Enter file descriptor:"<<endl;
                        cin>>fd;
                        if(isOpen(fd)){
                            if(open_files[fd] == 0){
                                if(read_file(fd) == -1){
                                    cout<<"Error while reading the file"<<endl;
                                }
                            }
                            else cout<<"Open the file in read mode"<<endl;
                        }
                        else cout<<"Please the file first."<<endl;
                    }
                    else if(j==4){
                        int fd;
                        cout<<"Enter file descriptor:"<<endl;
                        cin>>fd;
                        if(isOpen(fd)){
                            if(open_files[fd] == 2){
                                if(append_file(fd) == -1){
                                    cout<<"Error while reading the file"<<endl;
                                }
                            }
                            else cout<<"Open the file in append mode"<<endl;
                        }
                        else cout<<"Please open the file first."<<endl;
                    }
                    else if(j==5){   //write file
                        int fd;
                        cout<<"Enter file descriptor:"<<endl;
                        cin>>fd;
                        if(isOpen(fd)){
                            if(open_files[fd] == 1){
                                if(write_file(fd)){
		                    cout<<"Written successfully"<<endl;
		                }
		                else{
		                    cout<<"Error while writing into file"<<endl;
		                }
                            }
                            else cout<<"Open the file in write mode"<<endl;
                        }
                        else cout<<"Please open the file first."<<endl;
                    }
                    else if(j==6){   //close file
                       int fd;
                        cout<<"Enter file descriptor:"<<endl;
                        cin>>fd;
                        if(isOpen(fd)){
                            open_files.erase(fd);
                            cout<<"File closed successfully"<<endl;
                        }
                        else cout<<"File is not open."<<endl; 
                    }
                    else if(j==7){  //delete file
                        string filename;
                        cout<<"Enter the file name:"<<endl;
                        cin>>filename;
                        if(fName_to_fd.find(filename) != fName_to_fd.end()){
                            if(delete_file(filename)){
                                cout<<"File deleted successfully!"<<endl;
                            }
                            else cout<<"Error on file deletion!!!"<<endl;
                        }
                        else{
                            cout<<"File does not exist!!!"<<endl;
                        }
                    }
                    else if(j==8){ //List all files present in the current disk.
                        for(int i=0;i<NO_OF_INODES;i++){
                            if(Super_Block.i_bitmap[i] == 0){
                                cout<<Super_Block.fnameToiNodeMap[i].fileName<<"    "<<i<<endl;
                            }
                        }
                    }
                    else if(j==9){  //list of opened files
                        for(auto itr:open_files){
                            cout<<Super_Block.fnameToiNodeMap[itr.first].fileName<<"    "<<itr.first<<"     "<<file_modes[itr.second]<<endl;
                        }
                    }
                    else if(j==10){
                        unmount_disk(current_disk);
                        break;
                    }
                }
            }
        }
        else if(i==3){
            exit(0);
        }
    }
}