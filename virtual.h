#include <iostream>
#include <ostream>
#include <fstream>
#include <vector>
#include <cstring>

using namespace std;

class VirtualMachine
{
private:
    static const unsigned int BLOCK_SIZE = 2048;
    static const unsigned int INODE_NUM = 32;
    static const unsigned int INODE_CAPABILITY = 64;
    static const unsigned int BLOCK_NUM = 512;

    struct DataBlock
    {
        bool isFreeBlock;
        char data[2048];
    };
    struct Inode
    {
        std::size_t fileSize;
        char name[32];
        unsigned int inodeInFileNr;
        int nextInode;
        int pointers[INODE_CAPABILITY];
    };

    struct SuperBlock
    {
        int magicNumber;
        int blockSize = BLOCK_SIZE;
        int inodeNum = INODE_NUM;
        int inodeCapability = INODE_CAPABILITY;
        int blockNum = BLOCK_NUM;
        unsigned int freeInodes;
        unsigned int freeDataBlock;
    };
   const char * discName;
    SuperBlock mySuperBlock;

public:
    VirtualMachine(const char* image):discName(image)
    {};

    bool create()
    {

        mySuperBlock.magicNumber = 5555;
        mySuperBlock.freeDataBlock = BLOCK_NUM;
        mySuperBlock.freeInodes = INODE_NUM;
        Inode emptyInode;
        emptyInode.name[0] = '\0' ;
        emptyInode.fileSize = 0;
        emptyInode.inodeInFileNr = 0;
        DataBlock emptyBlock;
        emptyBlock.isFreeBlock = true;


        std::ofstream newFile(discName, std::ios::binary | std::ios::out);
        if(!newFile.is_open())
            cout<<"problem with creating disc"<<endl;
        newFile.seekp(0);
        newFile.write( (char*) &mySuperBlock, sizeof(mySuperBlock));
       // newFile.seekp(sizeof(mySuperBlock));
        for(auto i =0; i<INODE_NUM ; ++i)
        {
           // newFile.seekp(sizeof(mySuperBlock)+i* sizeof(Inode));
            newFile.write((char*) &emptyInode, sizeof(Inode));
        }
        for(auto i = 0; i<BLOCK_NUM;++i)
        {
           // newFile.seekp(sizeof(mySuperBlock)+INODE_NUM* sizeof(Inode)+i* sizeof(DataBlock));
            newFile.write((char*) &emptyBlock, sizeof(DataBlock));
        }

        cout<<newFile.tellp()<<endl;
        newFile.close();

    }

    bool writeToMachine(char* filename)
    {
        std::ifstream fin;
        fin.clear();
         fin.open(filename,std::ios_base::in|std::ios_base::binary);
            if(!fin)
            cout<<"cennot open file"<<endl;

       int fileSize = filesize(filename);
       char* bufor = new char[fileSize];
        fin.seekg (0, ios::beg);
        fin.read(bufor,fileSize);
        fin.close();

        std::ifstream readDisc;
        readDisc.clear();
                readDisc.open(discName,std::ios_base::in|std::ios_base::binary);
        std::ofstream writeDysk;
        writeDysk.clear();
                writeDysk.open(discName, std::ios_base::in|std::ios::out |std::ios::binary );
        if(!checkDisk())
            return false;
        readDisc.seekg(0,ios::beg);
        SuperBlock discSuperBlock;
        readDisc.read((char*)&discSuperBlock, sizeof(SuperBlock));
        if(discSuperBlock.freeDataBlock*BLOCK_SIZE < fileSize || discSuperBlock.freeInodes*INODE_CAPABILITY*BLOCK_SIZE < fileSize)
        {
            readDisc.close();
            writeDysk.close();
            cout<<"not enough space"<<endl;
            return false;

        }
        Inode discInode;
        auto Inodenr =0;
        auto remeningSize = fileSize;
        DataBlock discBlock;
        readDisc.seekg(sizeof(SuperBlock));
        int usedBlocs=0;
        for(auto i =0;i<INODE_NUM&&remeningSize>0;++i)
        {
            readDisc.seekg(sizeof(SuperBlock)+i* sizeof(Inode));
            readDisc.read((char*) &discInode, sizeof(Inode));
            if(discInode.fileSize==0)
            {
                for(auto m =0, n=0; m<INODE_CAPABILITY && n<BLOCK_NUM && remeningSize>0;++n)
                {

                    readDisc.seekg(sizeof(SuperBlock)+INODE_NUM* sizeof(Inode)+n* sizeof(DataBlock));
                    readDisc.read((char*) &discBlock, sizeof(DataBlock));
                    if(discBlock.isFreeBlock)
                    {
                        discBlock.isFreeBlock = false;
                        for(auto p =0; remeningSize>0 && p<BLOCK_SIZE;++p,remeningSize-=1)
                        {
                            discBlock.data[p]=bufor[usedBlocs*BLOCK_SIZE+p];
                        }
                        writeDysk.seekp(sizeof(SuperBlock)+INODE_NUM* sizeof(Inode)+n* sizeof(DataBlock));
                        writeDysk.write((char*) &discBlock, sizeof(DataBlock));
                        discInode.pointers[m]=n;
                        --discSuperBlock.freeDataBlock;
                        ++m;
                    }
                }
                Inodenr++;
                discInode.inodeInFileNr=Inodenr;
                discInode.fileSize = fileSize;
                for(auto i = 0; i<32;i++)
                    discInode.name[i] = filename[i];

                writeDysk.seekp(sizeof(SuperBlock)+i* sizeof(Inode));
                writeDysk.write((char*)&discInode, sizeof(Inode));
            }

        }
        discSuperBlock.freeInodes-=Inodenr;
        writeDysk.seekp(0);
        writeDysk.write((char*) &discSuperBlock, sizeof(SuperBlock));
        readDisc.close();
        writeDysk.close();
        delete bufor;
        return true;

    }

    bool viewFiles()
    {
        std::ifstream readDisc(discName,std::ios_base::in|std::ios_base::binary);
        if(!checkDisk())
            return false;
        for(auto i =0;i<INODE_NUM;++i)
        {
            Inode discInode;
            readDisc.seekg(sizeof(SuperBlock)+i* sizeof(Inode));
            readDisc.read((char*) &discInode, sizeof(Inode));
            if(discInode.fileSize>0 && discInode.inodeInFileNr==1)
            {
                cout <<discInode.name <<" size: "<<discInode.fileSize <<endl;
            }
        }
        return true;


    }

    bool deleteDisk()
    {

        remove(discName);
        return true;
    }

    bool deleteFile(char* deletingFile)
    {
        std::ifstream readDisc(discName,std::ios_base::in|std::ios_base::binary);
        std::ofstream writeDysk(discName, std::ios::binary | std::ios::out|std::ios::in);

        if(!checkDisk())
            return false;
        Inode discInode;
        readDisc.seekg(sizeof(SuperBlock));
        readDisc.seekg(0,ios::beg);
        SuperBlock discSuperBlock;
        int controlFileSize=1;
        readDisc.read((char*)&discSuperBlock, sizeof(SuperBlock));
        for(auto i =0;i<INODE_NUM&&controlFileSize>0;++i)
        {
            readDisc.seekg(sizeof(SuperBlock)+i* sizeof(Inode));
            readDisc.read((char*) &discInode, sizeof(Inode));
            if(!strcmp(discInode.name, deletingFile) && discInode.fileSize!=0 )
            {
                if(discInode.inodeInFileNr==1)
                    controlFileSize = discInode.fileSize;
                discInode.fileSize = 0;
                for(auto m =0; m<INODE_CAPABILITY&&controlFileSize>0;++m)
                {
                    discInode.pointers[m]=-1;
                    controlFileSize-=BLOCK_SIZE;
                    discSuperBlock.freeDataBlock+=1;
                }
                writeDysk.seekp(sizeof(SuperBlock)+i* sizeof(Inode));
                writeDysk.write((char*)&discInode, sizeof(Inode));
                discSuperBlock.freeInodes+=1;
            }
        }
        writeDysk.seekp(0);
        writeDysk.write((char*) &discSuperBlock, sizeof(SuperBlock));
        readDisc.close();
        writeDysk.close();
        return true;
    }
    std::ifstream::pos_type filesize(const char* filename)
    {
        std::ifstream in(filename, std::ifstream::ate | std::ifstream::binary);
        return in.tellg();
    }


    bool viewMemory()
    {


        if(!checkDisk())
            return false;
        std::ifstream readDisc(discName,std::ios_base::in|std::ios_base::binary);
        Inode discInode;
        SuperBlock discSuperBlock;
        DataBlock discBlock;
        readDisc.read((char*)&discSuperBlock, sizeof(SuperBlock));
        cout<<"SuperBlock adress: 0  size: "<< sizeof(discSuperBlock)<<endl;
        for(auto i =0;i<INODE_NUM;++i)
        {
            readDisc.seekg(sizeof(SuperBlock)+i* sizeof(Inode));
            readDisc.read((char*) &discInode, sizeof(Inode));
            cout<<"Inode nr: "<<i+1<<" adress: "<<sizeof(SuperBlock)+i* sizeof(Inode)<< " size: "<<sizeof(Inode)<<" pointing file";
                if(discInode.fileSize>0)
                    cout<<discInode.name;
                else
                    cout<<"none";
                       cout <<" file size: "<<discInode.fileSize<<endl;
        }
        for(auto i =0;i<BLOCK_NUM;++i)
        {
            readDisc.seekg(sizeof(SuperBlock)+INODE_NUM* sizeof(Inode)+i* sizeof(DataBlock));
            readDisc.read((char*) &discBlock, sizeof(DataBlock));
            cout<<"DataBlock nr: "<<i+1<<" adress: "<<sizeof(SuperBlock)+INODE_NUM* sizeof(Inode)+i* sizeof(DataBlock)<< " size: "<<sizeof(DataBlock)<<" isfree: "<<discBlock.isFreeBlock<<endl;
        }
        return true;

    }

    bool copyFromMachine(char* filename)
    {


       if(!checkDisk())
           return false;

        Inode discInode;
        int remeningSize =1;
        int sizeofFile;
        DataBlock discBlock;
        SuperBlock discSuperBlock;
        int blocksUsed =0;
        char* bufor;

        std::ifstream readDisc(discName,std::ios_base::in|std::ios_base::binary);
        std::ofstream writeDysk(discName, std::ios::binary | std::ios::out|std::ios::in);
        readDisc.seekg(0);
        readDisc.read((char*)&discSuperBlock, sizeof(SuperBlock));


        for(auto i =0;i<INODE_NUM&&remeningSize>0;++i)
        {
            readDisc.seekg(sizeof(SuperBlock)+i* sizeof(Inode));
            readDisc.read((char*) &discInode, sizeof(Inode));
            if(discInode.fileSize!=0&&!strcmp(discInode.name, filename))
            {
                if(discInode.inodeInFileNr==1)
                {
                    sizeofFile = remeningSize=discInode.fileSize;
                    bufor = new char[discInode.fileSize];

                }

                for(auto n=0; n<INODE_CAPABILITY &&  remeningSize>0;++n)
                {
                    int dataPtr = discInode.pointers[n];

                    readDisc.seekg(sizeof(SuperBlock)+INODE_NUM* sizeof(Inode)+dataPtr* sizeof(DataBlock));
                    readDisc.read((char*) &discBlock, sizeof(DataBlock));

                    for(auto p =0; remeningSize>0 && p<BLOCK_SIZE;++p,remeningSize-=1)
                    {
                        bufor[blocksUsed*BLOCK_SIZE+p]=discBlock.data[p];
                    }
                    discBlock.isFreeBlock=false;
                    writeDysk.seekp(sizeof(SuperBlock)+INODE_NUM* sizeof(Inode)+dataPtr* sizeof(DataBlock));
                    writeDysk.write((char*)&discBlock, sizeof(DataBlock));
                    discSuperBlock.freeDataBlock++;
                    ++blocksUsed;
                }
                discSuperBlock.freeInodes++;
                discInode.inodeInFileNr=0;
                discInode.fileSize = 0;

                writeDysk.seekp(sizeof(SuperBlock)+i* sizeof(Inode));
                writeDysk.write((char*)&discInode, sizeof(Inode));
            }

        }
        writeDysk.seekp(0);
        writeDysk.write((char*) &discSuperBlock, sizeof(SuperBlock));
        readDisc.close();
        writeDysk.close();
        std::ofstream newFile(filename, std::ios::binary | std::ios::out);
        if(!newFile.is_open())
            cout<<"problem with creating file"<<endl;
        newFile.seekp(0);

        for(auto i =0;i<sizeofFile;++i)
            newFile.write(&bufor[i], sizeof(char));
        newFile.close();
        delete bufor;


        return true;

    }
    bool checkDisk()
    {
        std::ifstream readDisc;
        readDisc.clear();
        readDisc.open(discName,std::ios_base::in|std::ios_base::binary);
        std::ofstream writeDysk;
        writeDysk.clear();
        writeDysk.open(discName, std::ios_base::in|std::ios::out |std::ios::binary );
        if(!readDisc.is_open()||!writeDysk.is_open())
        {
            readDisc.close();
            writeDysk.close();
            cout<<"problem with opening disk::writing"<<endl;
            return false;
        }
        readDisc.seekg(0);
        int check;
        readDisc.read((char*)&check, sizeof(int));
        if( check !=mySuperBlock.magicNumber)
        {
            readDisc.close();
            writeDysk.close();
            cout<<"problem with reading disk"<<check<<endl;
            return false;
        }
        readDisc.close();
        writeDysk.close();
        return true;
    };




};




