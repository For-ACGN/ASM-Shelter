package main

import (
	"context"
	"fmt"
	"log"
	"math/rand"
	"reflect"
	"syscall"
	"time"
	"unsafe"
)

var (
	modKernel32            = syscall.NewLazyDLL("kernel32.dll")
	procGetCurrentThreadID = modKernel32.NewProc("GetCurrentThreadId")
	procSleep              = modKernel32.NewProc("Sleep")
)

func main() {
	testRuntimeAPI()
	testMemoryData()
	testGoRoutine()
	testLargeBuffer()
	kernel32Sleep()
	
	for {
		time.Sleep(time.Second)
	}
}

func testRuntimeAPI() {
	dll := syscall.NewLazyDLL("kernel32.dll")
	hModule := syscall.Handle(dll.Handle())
	GetProcAddress := dll.NewProc("GetProcAddress").Addr()
	fmt.Printf("GetProcAddress: 0x%X\n", GetProcAddress)
	
	for _, proc := range []string{
		"RT_GetProcAddressByName",
		"RT_GetProcAddressByHash",
		"RT_GetProcAddressOriginal",
	} {
		err := dll.NewProc(proc).Find()
		if err != nil {
			fmt.Println("[warning] failed to find runtime methods")
			return
		}
		dllProcAddr := dll.NewProc(proc).Addr()
		getProcAddr, err := syscall.GetProcAddress(hModule, proc)
		checkError(err)
		if dllProcAddr != getProcAddr {
			log.Fatalln("unexpected proc address")
		}
		fmt.Printf("%s: 0x%X\n", proc, dllProcAddr)
	}
	fmt.Println()
	
	GetProcAddressOriginal, err := syscall.GetProcAddress(hModule, "RT_GetProcAddressOriginal")
	checkError(err)
	
	// get original GetProcAddress
	proc, err := syscall.BytePtrFromString("GetProcAddress")
	checkError(err)
	ret, _, _ := syscall.SyscallN(
		GetProcAddressOriginal,
		uintptr(hModule), (uintptr)(unsafe.Pointer(proc)),
	)
	if ret == 0 {
		log.Fatalln("failed to get GetProcAddress address")
	}
	fmt.Printf("Original GetProcAddress: 0x%X\n", ret)
	fmt.Printf("Hooked   GetProcAddress: 0x%X\n", GetProcAddress)
	
	// get original VirtualAlloc
	proc, err = syscall.BytePtrFromString("VirtualAlloc")
	checkError(err)
	ret, _, _ = syscall.SyscallN(
		GetProcAddressOriginal,
		uintptr(hModule), (uintptr)(unsafe.Pointer(proc)),
	)
	if ret == 0 {
		log.Fatalln("failed to get GetProcAddress address")
	}
	
	VirtualAlloc, err := syscall.GetProcAddress(hModule, "VirtualAlloc")
	checkError(err)
	
	fmt.Printf("Original VirtualAlloc: 0x%X\n", ret)
	fmt.Printf("Hooked   VirtualAlloc: 0x%X\n", VirtualAlloc)
}

var globalVar = 12345678

func testMemoryData() {
	go func() {
		localVar := 12121212
		localStr := "hello GleamRT"
		
		for {
			tid, _, _ := procGetCurrentThreadID.Call()
			fmt.Println("Thread ID:", tid)
			
			fmt.Printf("global variable pointer: 0x%X\n", &globalVar)
			fmt.Println("global variable value:  ", globalVar)
			
			fmt.Printf("local variable pointer:  0x%X\n", &localVar)
			fmt.Println("local variable value:   ", localVar)
			
			funcAddr := reflect.ValueOf(testRuntimeAPI).Pointer()
			fmt.Printf("instruction:             0x%X\n", funcAddr)
			
			inst := unsafe.Slice((*byte)(unsafe.Pointer(funcAddr)), 8)
			fmt.Printf("instruction data:        %v\n", inst)
			
			time.Sleep(time.Second)
			fmt.Println(localStr, "finish!")
			fmt.Println()
		}
	}()
}

func testGoRoutine() {
	ch := make(chan int, 1024)
	ctx, cancel := context.WithCancel(context.Background())
	go func() {
		var i int
		for {
			select {
			case ch <- i:
			case <-ctx.Done():
				return
			}
			i++
			time.Sleep(100 * time.Millisecond)
		}
	}()
	go func() {
		// deadlock
		defer cancel()
		for {
			select {
			case i := <-ch:
				fmt.Println("index:", i)
			case <-ctx.Done():
				return
			}
		}
	}()
}

func testLargeBuffer() {
	alloc := func(min, max int) {
		for {
			buf := make([]byte, min+rand.Intn(max))
			init := byte(rand.Int())
			for i := 0; i < len(buf); i++ {
				buf[i] = init
				init++
			}
			fmt.Println("alloc buffer:", len(buf))
			time.Sleep(250 * time.Millisecond)
		}
	}
	go alloc(1, 128)
	go alloc(1, 512)
	go alloc(256, 1024)
	go alloc(512, 1024)
	go alloc(1024, 16*1024)
	go alloc(4096, 16*1024)
	go alloc(16*1024, 512*1024)
	go alloc(64*1024, 512*1024)
	go alloc(1*1024*1024, 8*1024*1024)
	go alloc(2*1024*1024, 8*1024*1024)
}

func kernel32Sleep() {
	go func() {
		for {
			// wait go routine run test
			time.Sleep(5 * time.Second)
			
			// trigger Gleam-RT Sleep
			fmt.Println("call kernel32.Sleep [hooked]")
			now := time.Now()
			ok, _, _ := procSleep.Call(100)
			if ok != 1 {
				log.Fatalln("occurred when sleep")
			}
			fmt.Println("Sleep:", time.Since(now))
			fmt.Println()
		}
	}()
}

func checkError(err error) {
	if err != nil {
		log.Fatalln(err)
	}
}
