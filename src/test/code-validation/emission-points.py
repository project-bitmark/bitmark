#!/usr/bin/python

reward_start = 2000000000
quartering_interval = 394000

reward_cur = reward_start
emitted_cur = reward_cur*quartering_interval

print("if (emitted < "+str(emitted_cur)+") {")
print("\tbaseSubsidy = "+str(reward_cur)+";")
print("}")

for i in range(0,34):
    if i%2:
        reward_cur = int((reward_cur*2)/3)
    else:
        reward_cur = int((reward_cur*3)/4)    
    emitted_cur += reward_cur*quartering_interval
    print("else if (emitted < "+str(emitted_cur)+") {")
    print("\tbaseSubsidy = "+str(reward_cur)+";")
    print("}")
