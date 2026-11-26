#include "headers.h"
#include "diskmgr.h"
#include <array>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>

namespace {
using Bytes = std::vector<uint8>;
constexpr unsigned headerSize = 0x2b0, sectorSize = 512, sectors = 10;
void require(bool ok, const char* why) {
    if (!ok) throw std::runtime_error(why);
}
void put16(Bytes& b, size_t o, unsigned v) {
    b[o] = uint8(v); b[o + 1] = uint8(v >> 8);
}
void put32(Bytes& b, size_t o, unsigned v) {
    for (int i = 0; i < 4; ++i) b[o+i] = uint8(v >> (8*i));
}
unsigned get32(const Bytes& b, size_t o) {
    unsigned v = 0;
    for (int i = 0; i < 4; ++i) v |= unsigned(b.at(o+i)) << (8*i);
    return v;
}
Bytes image(unsigned type, bool readonly = false, int missing = -1, int shortTrack = -1) {
    Bytes b(headerSize, 0);
    memcpy(b.data(), "Writeback test", 14);
    b[26] = readonly ? 0x10 : 0;
    b[27] = uint8(type);
    unsigned tracks = type == 0 ? 84 : 164;
    for (unsigned t = 0; t < tracks; ++t) {
        if (int(t) == missing) continue;
        put32(b, 32+4*t, unsigned(b.size()));
        unsigned count = int(t) == shortTrack ? 1 : sectors;
        for (unsigned r = 1; r <= count; ++r) {
            size_t o = b.size(); b.resize(o+16+sectorSize, 0);
            b[o] = uint8(t/2); b[o+1] = uint8(t&1); b[o+2] = uint8(r); b[o+3] = 2;
            put16(b, o+4, count); put16(b, o+14, sectorSize);
            std::fill(b.begin()+o+16, b.end(), uint8(t+r));
        }
    }
    put32(b, 28, unsigned(b.size())); return b;
}
void save(const std::filesystem::path& p, const Bytes& b) {
    require(!std::filesystem::exists(p), "test must use a fresh file");
    std::ofstream f(p, std::ios::binary);
    f.write(reinterpret_cast<const char*>(b.data()), b.size());
    require(bool(f), "fixture write");
}
Bytes read(const std::filesystem::path& p) {
    std::ifstream f(p, std::ios::binary);
    require(bool(f), "readback open");
    return Bytes(std::istreambuf_iterator<char>(f), {});
}
unsigned flags(unsigned type, unsigned t) {
    return PC8801::FDU::MFM | (t & 1) | (type == 0x20 ? 0x80 : 0);
}
unsigned cylinder(unsigned type, unsigned t) { return (t/2)*(type == 0 ? 2 : 1); }
void write(DiskManager& d, unsigned type, unsigned t, unsigned r, uint8 fill, Bytes& expected) {
    auto f = d.GetFDU(0);
    require(f->Seek(cylinder(type,t)) == 0, "seek");
    FloppyDisk::IDR id{uint8(t/2),uint8(t&1),uint8(r),2};
    std::array<uint8,sectorSize> payload; payload.fill(fill);
    require(f->WriteSector(flags(type,t),id,payload.data(),false)==0,"write sector");
    auto pos = get32(expected,32+4*t)+16+(r-1)*(16+sectorSize);
    std::copy(payload.begin(),payload.end(),expected.begin()+pos);
}
void incremental(const std::filesystem::path& dir, unsigned type) {
    auto p = dir / ("incremental-" + std::to_string(type) + ".d88");
    auto expected = image(type);
    // A full image rewrite would erase this reserved header byte.
    expected[25] = 0x5a;
    save(p, expected);
    DiskManager d; require(d.Init(),"init");
    require(d.Mount(0,p.string().c_str(),false,0,false),"mount");
    std::vector<unsigned> tracks = type == 0 ? std::vector<unsigned>{29,28,57,79,82,83,0,1} : std::vector<unsigned>{29,28,57,79,162,163,0,1};
    for (auto t : tracks) {
        write(d,type,t,2,0xd1,expected);
        d.Update();
        require(read(p)==expected,"first incremental update not visible while mounted");
        write(d,type,t,8,0xe2,expected);
        d.Update();
        require(read(p)==expected,"second incremental update not visible while mounted");
    }
    require(d.Unmount(0),"unmount");
    require(read(p)==expected,"incremental image differs (wrong track or unintended full rewrite)");
    require(d.Mount(0,p.string().c_str(),false,0,false),"remount");
    auto f=d.GetFDU(0);
    for(auto t: tracks) {
        std::array<uint8,sectorSize> got{};
        FloppyDisk::IDR id{uint8(t/2),uint8(t&1),8,2};
        f->Seek(cylinder(type,t));
        require(f->ReadSector(flags(type,t),id,got.data())==0,"read sector");
        require(std::all_of(got.begin(),got.end(),[](uint8 v){return v==0xe2;}),"reload payload");
    }
    require(d.Unmount(0),"unmount reloaded");
    printf("PASS incremental type 0x%02x: both heads, repeated writes, edge tracks, exact image and remount\n",type);
}
void resizeTrack(const std::filesystem::path& dir, unsigned type, bool missing, bool shrink) {
    unsigned t=29, count=shrink?1:sectors;
    auto p=dir/("format-"+std::to_string(type)+"-"+std::to_string(missing)+"-"+std::to_string(shrink)+".d88");
    auto before=image(type,false,missing?int(t):-1,(!missing&&!shrink)?int(t):-1);
    save(p,before);
    DiskManager d; require(d.Init(),"init");
    require(d.Mount(0,p.string().c_str(),false,0,false),"mount format");
    auto f=d.GetFDU(0); f->Seek(cylinder(type,t));
    std::vector<FloppyDisk::IDR> ids;
    for(unsigned r=1;r<=count;r++)ids.push_back({uint8(t/2),uint8(t&1),uint8(r),2});
    PC8801::FDU::WIDDESC desc{ids.data(),2,uint8(count),0,0x7a};
    require(f->WriteID(flags(type,t),&desc)==0,"format");
    d.Update();
    require(d.Unmount(0),"unmount format");
    auto expected=image(type,false,-1,shrink?int(t):-1);
    auto pos=get32(expected,32+4*t);
    for(unsigned r=0;r<count;r++)
        std::fill_n(expected.begin()+pos+r*(16+sectorSize)+16,sectorSize,uint8(0x7a));
    require(read(p)==expected,"format image differs");
    printf("PASS format type 0x%02x: %s\n",type,missing?"missing track":shrink?"shrink":"grow");
}
void protectedDisk(const std::filesystem::path& dir,unsigned type) {
    auto p=dir/("protected-"+std::to_string(type)+".d88");
    auto original=image(type,true);save(p,original);
    DiskManager d;require(d.Init(),"init");
    require(d.Mount(0,p.string().c_str(),false,0,false),"mount protected");
    auto f=d.GetFDU(0);f->Seek(cylinder(type,29));
    FloppyDisk::IDR id{14,1,1,2}; std::array<uint8,sectorSize> bytes{};
    require(f->WriteSector(flags(type,29),id,bytes.data(),false)!=0,"write protection");
    d.Update();require(d.Unmount(0),"unmount protected");
    require(read(p)==original,"protected disk changed");
    printf("PASS protected type 0x%02x\n",type);
}
void combined(const std::filesystem::path& dir, bool shrink) {
    const unsigned t=29;
    auto p=dir/(shrink?"combined-shrink.d88":"combined-grow.d88");
    auto first=image(0,false,-1,shrink?-1:int(t));
    auto second=image(0x10),third=image(0x20);
    auto initial=first;
    initial.insert(initial.end(),second.begin(),second.end());
    initial.insert(initial.end(),third.begin(),third.end());
    save(p,initial);
    DiskManager d;require(d.Init(),"init combined");
    require(d.Mount(0,p.string().c_str(),false,0,false),"mount bank 0");
    require(d.Mount(1,p.string().c_str(),false,1,false),"mount bank 1");
    auto f=d.GetFDU(0);f->Seek(cylinder(0,t));
    unsigned count=shrink?1:sectors;
    std::vector<FloppyDisk::IDR> ids;
    for(unsigned r=1;r<=count;r++)ids.push_back({14,1,uint8(r),2});
    PC8801::FDU::WIDDESC desc{ids.data(),2,uint8(count),0,0x7a};
    require(f->WriteID(flags(0,t),&desc)==0,"format bank 0");
    d.Update();
    require(d.Unmount(0),"unmount bank 0");
    // Bank 1 is still mounted: use its updated logical origin for incremental writeback.
    auto f1=d.GetFDU(1);f1->Seek(cylinder(0x10,t));
    std::array<uint8,sectorSize> bytes;bytes.fill(0x55);
    FloppyDisk::IDR id{14,1,1,2};
    require(f1->WriteSector(flags(0x10,t),id,bytes.data(),false)==0,"write bank 1");
    auto pos=get32(second,32+4*t)+16;
    std::copy(bytes.begin(),bytes.end(),second.begin()+pos);
    d.Update();require(d.Unmount(1),"unmount bank 1");
    auto expected=image(0,false,-1,shrink?int(t):-1);
    pos=get32(expected,32+4*t);
    for(unsigned r=0;r<count;r++)
        std::fill_n(expected.begin()+pos+r*(16+sectorSize)+16,sectorSize,uint8(0x7a));
    expected.insert(expected.end(),second.begin(),second.end());
    expected.insert(expected.end(),third.begin(),third.end());
    require(read(p)==expected,"combined image differs after resize and write to next bank");
    printf("PASS combined three banks: %s, mounted next bank write, untouched last bank\n",shrink?"shrink":"grow");
}
void oddCylinder(const std::filesystem::path& dir) {
    auto p=dir/"odd-physical-cylinder.d88";auto original=image(0);save(p,original);
    DiskManager d;require(d.Init(),"init");
    require(d.Mount(0,p.string().c_str(),false,0,false),"mount odd");
    auto f=d.GetFDU(0);f->Seek(15);
    FloppyDisk::IDR id{15,1,1,2};
    PC8801::FDU::WIDDESC desc{&id,2,1,0,0x7a};
    require(f->WriteID(flags(0,1),&desc)==0,"odd format");
    d.Update();require(d.Unmount(0),"unmount odd");
    require(read(p)==original,"odd physical cylinder aliased into logical D88");
    puts("PASS odd physical cylinder does not alias into 2D D88 (not representable)");
}
}
int main(int argc,char** argv) {
    try {
        require(argc==2,"usage: d88_writeback_test NEW_FIXTURE_DIRECTORY");
        std::filesystem::path dir=argv[1];
        require(std::filesystem::create_directory(dir),"fresh fixture directory required");
        for (unsigned type : {0u,0x10u,0x20u}) {
            incremental(dir,type);protectedDisk(dir,type);
            resizeTrack(dir,type,true,false);resizeTrack(dir,type,false,false);resizeTrack(dir,type,false,true);
        }
        oddCylinder(dir);
        combined(dir,false);combined(dir,true);
        puts("PASS all 18 cases");return 0;
    } catch(const std::exception& e) { fprintf(stderr,"FAIL: %s\n",e.what());return 1; }
}
