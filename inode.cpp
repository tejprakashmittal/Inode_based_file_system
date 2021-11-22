#include<bits/stdc++.h>
#define TOTAL_BLOCKS 5
#define BLOCK_SIZE 4096
#define NO_OF_DATA_BLOCKS 2
#define NO_OF_INODES 2
#define INODE_START_INDEX 1
#define DATA_BLOCK_START_INDEX 3
using namespace std;

string current_disk;
FILE *disk_pointer;

struct iNode{   // total size 8 bytes
    int fileSize; // 4 bytes
    int dataBlock;  //4bytes
};

struct fileName_To_iNode_map{ //total size 24 bytes
    char fileName[20]; //max 20 bytes
    int iNode_index; // 4 bytes   inode number == inode index == fileDescriptor
};

struct superBlock{  //64 B
    int i_bitmap[NO_OF_INODES];    //8 B
    int d_bitmap[NO_OF_DATA_BLOCKS]; // 8 B
    struct fileName_To_iNode_map fnameToiNodeMap[NO_OF_INODES]; // 24*2=48 B
    struct iNode iNode_array[NO_OF_INODES];
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
   cout<<Super_Block.i_bitmap[1]<<endl;
   cout<<Super_Block.d_bitmap[1]<<endl;
   return 1;
}

int unmount_disk(string diskname){
   disk_pointer = fopen(diskname.c_str(),"rb+"); 
   char buffer[BLOCK_SIZE];
   bzero(buffer,BLOCK_SIZE);
   bzero(buffer,BLOCK_SIZE);
   memcpy(buffer,&Super_Block,sizeof(Super_Block));
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
    cout<<"Broke1"<<endl;
    for(int i=0;i<NO_OF_DATA_BLOCKS;i++){
        if(Super_Block.d_bitmap[i] == 1){
            d_index = i;
            break;
        }
    }
    cout<<"Broke2"<<endl;
    if(i_index == -1 || d_index == -1){
        cout<<"inode_index "<<i_index<<endl<<"datablock "<<d_index<<endl;
        return -1;
    }
    Super_Block.iNode_array[i_index].dataBlock = d_index + DATA_BLOCK_START_INDEX; // actual datablock index
    cout<<"Broke3"<<endl;
    Super_Block.i_bitmap[i_index]=0;
    cout<<"Broke4"<<endl;
    strcpy(Super_Block.fnameToiNodeMap[i_index].fileName,filename.c_str());
    cout<<"Broke5"<<endl;
    Super_Block.fnameToiNodeMap[i_index].iNode_index = i_index + INODE_START_INDEX; // actual index
    cout<<"Broke6"<<endl;
    Super_Block.d_bitmap[d_index]=0;
    cout<<"Broke7"<<endl;
    //fclose(disk_pointer);
    return i_index + INODE_START_INDEX;  //actual index
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
                    cout<<"6: unmount disk"<<endl;
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

                    }
                    else if(j==3){

                    }
                    else if(j==4){

                    }
                    else if(j==5){

                    }
                    else if(j==6){
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