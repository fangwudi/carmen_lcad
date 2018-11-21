#
# Main script for training a model for semantic segmentation of 
# roads from Velodyne point clouds
#
# Andre Seidel Oliveira
#

from __future__ import print_function
import argparse
import torch
import torch.nn as nn
import torch.nn.functional as F
import torch.legacy.nn as nn2
import torch.optim as optim
from torchvision import datasets, transforms
from PIL import Image
import math
import model as M
from random import shuffle
import matplotlib.pyplot as plt
import numpy as np
import cv2

n_imgs = 900
n_train = 280
n_test = 120
#n_train = 10
#n_test = 1

train_start_index = 0
test_start_index = n_train

img_x_dim = 500
img_y_dim = 500

data_path = '/dados/neural_mapper_png_dataset/volta_da_ufes30102018/data/'
target_path = '/dados/neural_mapper_png_dataset/volta_da_ufes30102018/labels/'
debug_img_path = 'debug_imgs/'

input_dimensions = 5
n_classes = 3

def png2tensor(file_name):
    img2tensor = transforms.ToTensor()
    img = Image.open(file_name)
    return img2tensor(img)

def png2target(file_name):
    img2tensor = transforms.ToTensor()
    img = Image.open(file_name)
    #print((img2tensor(img)*255 - 1))
    target = (img2tensor(img)*255 - 1)

    weight = np.array([len(np.where(target.numpy() == t)[0]) for t in np.unique(target.numpy())])

    #print(weight)
    return target, weight
    #return img2tensor(img)

def tensor2png(tensor):
    trans = transforms.ToPILImage()
    return trans(tensor)

def tensor2rgbimage(tensor):
    img = tensor.permute(1,2,0).numpy()
    height, width = img.shape[:2]
    img_map = np.zeros((width,height,3), np.uint8)
    #print(np.where((img == [1]).all(axis = 2)))
    img_map[np.where((img == [0]).all(axis = 2))] = np.array([255,120,0])
    img_map[np.where((img == [1]).all(axis = 2))] = np.array([0,0,0])
    img_map[np.where((img == [2]).all(axis = 2))] = np.array([255,255,255])

    return img_map

def showOutput(tensor):
    img = tensor2png(tensor)
    img.show()

def saveImage(tensor, file_name):
    img = tensor2png(tensor)
    img.save(file_name)

def load_data(batch_size, dataset_size):
    dataset = []
    weights = np.zeros(n_classes)
    n = math.floor(dataset_size/batch_size)
    for i in range(n):
        data = torch.zeros(batch_size, input_dimensions, img_x_dim, img_y_dim)
        target = torch.zeros(batch_size, img_x_dim, img_y_dim, dtype=torch.int64)

        for j in range(batch_size):
            # + 1 se indice comeca em 1 
            data[j][0] = png2tensor(data_path + str(batch_size*i + j + train_start_index) + '_max.png')# + 1) + '_max.png')
            data[j][1] = png2tensor(data_path + str(batch_size*i + j + train_start_index) + '_mean.png')# + 1) + '_mean.png')
            data[j][2] = png2tensor(data_path + str(batch_size*i + j + train_start_index) + '_min.png')# + 1) + '_min.png')
            data[j][3] = png2tensor(data_path + str(batch_size*i + j + train_start_index) + '_numb.png')# + 1) + '_numb.png')
            data[j][4] = png2tensor(data_path + str(batch_size*i + j + train_start_index) + '_std.png')# + 1) + '_std.png')
            target[j], new_weights = png2target(target_path + str(batch_size*i + j + train_start_index) + '_label.png')
            weights = weights + new_weights

        row = []
        row.append(data)
        row.append(target)
        dataset.append(row)
        #print(weights)
    max_weight = max(weights)
    weights = max_weight/weights
    return dataset, weights

def load_train_data(batch_size, train_data_size):
    dataset = []

    n = math.floor(train_data_size/batch_size)
    for i in range(n):
        data = torch.zeros(batch_size, input_dimensions, img_x_dim, img_y_dim)
        target = torch.zeros(batch_size, img_x_dim, img_y_dim)

        for j in range(batch_size):
            data[j][0] = png2tensor(data_path + str(batch_size*i + j + train_start_index + 1) + '_max.png')
            data[j][1] = png2tensor(data_path + str(batch_size*i + j + train_start_index + 1) + '_mean.png')
            data[j][2] = png2tensor(data_path + str(batch_size*i + j + train_start_index + 1) + '_min.png')
            data[j][3] = png2tensor(data_path + str(batch_size*i + j + train_start_index + 1) + '_numb.png')
            data[j][4] = png2tensor(data_path + str(batch_size*i + j + train_start_index + 1) + '_std.png')
            target[j] = png2tensor(target_path + str(batch_size*i + j + train_start_index) + '_view.png')

        row = []
        row.append(data)
        row.append(target)
        dataset.append(row)
    return dataset


def load_test_data(batch_size, test_data_size):
    dataset = []

    n = math.floor(test_data_size/batch_size)
    for i in range(n):
        data = torch.zeros(batch_size, input_dimensions, img_x_dim, img_y_dim)
        target = torch.zeros(batch_size, img_x_dim, img_y_dim)

        for j in range(batch_size):
            data[j][0] = png2tensor(data_path + str(batch_size*i + j + test_start_index + 1) + '_max.png')
            data[j][1] = png2tensor(data_path + str(batch_size*i + j + test_start_index + 1) + '_mean.png')
            data[j][2] = png2tensor(data_path + str(batch_size*i + j + test_start_index + 1) + '_min.png')
            data[j][3] = png2tensor(data_path + str(batch_size*i + j + test_start_index + 1) + '_numb.png')
            data[j][4] = png2tensor(data_path + str(batch_size*i + j + test_start_index + 1) + '_std.png')
            target[j] = png2tensor(target_path + str(batch_size*i + j + test_start_index) + '_view.png')

        row = []
        row.append(data)
        row.append(target)
        dataset.append(row)
    return dataset

def train(args, model, device, train_loader, optimizer, epoch, weights):
    model.train()
    for batch_idx, (data, target) in enumerate(train_loader):
        data, target = data.to(device), target.long().to(device)
        optimizer.zero_grad()
        output = model(data)
        out_loss = F.cross_entropy(output, target)#, weight=weights)
        out_loss.backward()
        optimizer.step()
        if batch_idx % args.log_interval == 0:
            print('Train Epoch: {} [{}/{} ({:.0f}%)]\tLoss: {:.6f}'.format(
    #                epoch, batch_idx * len(data), len(train_loader.dataset),
                epoch, batch_idx * len(data), len(train_loader)*args.batch_size,
                100. * batch_idx / len(train_loader), out_loss.item()))

           # pred = output.max(1, keepdim=True)[1] # get the index of the max log-probability
            #imgPred = pred[0]
            #imgPred = imgPred.cpu().float()
            #imgTarget = torch.FloatTensor(1, 424, 424)
            #imgTarget[0] = target[0]
            #imgTarget = imgTarget.cpu().float()
            #saveImage(imgPred, debug_img_path + '/predic_epoch' + str(epoch) + '.png')
            #saveImage(imgTarget, debug_img_path + '/target_epoch' + str(epoch) + '.png')
            #showOutput(imgPred)
            #showOutput(imgTarget)

def test(args, model, device, test_loader, epoch):
    model.eval()
    test_loss = 0
    correct = 0
    with torch.no_grad():
        for data, target in test_loader:
            data, target = data.to(device), target.long().to(device)
            output = model(data)
            test_loss += F.cross_entropy(output, target, size_average=False).item() # sum up batch loss
            pred = output.max(1, keepdim=True)[1] # get the index of the max log-probability
            correct += pred.eq(target.view_as(pred)).sum().item()

    #    test_loss /= len(test_loader.dataset)
    test_loss /= (len(test_loader)*test_loader[0][1].size(0)*test_loader[0][1].size(1)*test_loader[0][1].size(2))
    print('\nTest set: Average loss: {:.4f}, Accuracy: {}/{} ({:.0f}%)\n'.format(
        test_loss, correct, len(test_loader)*test_loader[0][1].size(0)*test_loader[0][1].size(1)*test_loader[0][1].size(2),
        100. * correct / (len(test_loader)*test_loader[0][1].size(0)*test_loader[0][1].size(1)*test_loader[0][1].size(2))))

'''
    if(args.show_plots):
        update_plot(test_loss_plot, epoch, test_loss)


def update_plot(plot, epoch, new_loss):
    plot.set_xdata(numpy.append(plot.get_xdata(), epoch))
    plot.set_ydata(numpy.append(plot.get_ydata(), new_loss))
    plt.show()
'''

if __name__ == '__main__':
    # Training settings
    parser = argparse.ArgumentParser(description='PyTorch Neural Mapper')
    parser.add_argument('--batch-size', type=int, default=5, metavar='N',
                        help='input batch size for training (default: 64)')
    parser.add_argument('--test-batch-size', type=int, default=2, metavar='N',
                        help='input batch size for testing (default: 1000)')
    parser.add_argument('--epochs', type=int, default=100, metavar='N',
                        help='number of epochs to train (default: 10)')
    parser.add_argument('--lr', type=float, default=0.001, metavar='LR',
                        help='learning rate (default: 0.01)')
    parser.add_argument('--momentum', type=float, default=0.5, metavar='M',
                        help='SGD momentum (default: 0.5)')
    parser.add_argument('--no-cuda', action='store_true', default=False,
                        help='disables CUDA training')
    parser.add_argument('--seed', type=int, default=1, metavar='S',
                        help='random seed (default: 1)')
    parser.add_argument('--log-interval', type=int, default=10, metavar='N',
                        help='how many batches to wait before logging training status')
    parser.add_argument('--use-model', type=str, default=None, metavar='N',
                        help='Enter the name of a trained model (.model file)')
    parser.add_argument('--show-plots', action='store_true', default=False,
                        help='Enables loss plots')
    parser.add_argument('--training-logs', action='store_true', default=True,
                        help='Enables loss plots')

    args = parser.parse_args()
    use_cuda = not args.no_cuda and torch.cuda.is_available()

    torch.manual_seed(args.seed)

    device = torch.device("cuda" if use_cuda else "cpu")

    kwargs = {'num_workers': 1, 'pin_memory': True} if use_cuda else {}

    # Training and testing loss plots
    #test_loss_plot, = plt.plot(range(1, args.epochs), [], 'bs')


    #train_loader = load_train_data(args.batch_size, n_train)
    #test_loader = load_test_data(args.test_batch_size, n_test)

    dataset_loader, weights = load_data(args.batch_size, n_imgs)
    shuffle(dataset_loader)
    n_train_batches = math.floor(n_train/args.batch_size)
    train_loader = dataset_loader[:n_train_batches]    
    test_loader = dataset_loader[n_train_batches:]

    print("Train loader dataset size: " + str(len(train_loader)))
    print("Train loader data dimensions: " + str(train_loader[0][0].size()), "Train loader target dimensions: " + str(train_loader[0][1].size()))

    print("Test loader dataset size: " + str(len(test_loader)))    
    print("Test loader data dimensions: " + str(test_loader[0][0].size()), "Test loader target dimensions: " + str(test_loader[0][1].size()))

    model = M.FCNN(n_input=input_dimensions, n_output=n_classes).to(device)
    if(args.use_model is not None):
        model.load_state_dict(torch.load('saved_models/'+args.use_model))
        print("Using model " + args.use_model)
    
    optimizer = optim.Adam(model.parameters(), lr=args.lr)

    #weights[1] = weights[1]*50
    print("PESOS:")
    print (weights)
    #weights = [1., 100.]
    class_weights = torch.FloatTensor(weights).cuda()
    for epoch in range(1, args.epochs + 1):
        train(args, model, device, train_loader, optimizer, epoch, class_weights)
        test(args, model, device, test_loader, epoch)
        if(epoch % 10 == 0):
            torch.save(model.state_dict(), 'saved_models/' + str(n_imgs)  + "imgs_epoch" + str(epoch) + '.model')

'''
    data = train_loader[0][0]
    target = train_loader[0][1]
    data, target = data.to(device), target.long().to(device)
    output = model(data)
    pred = output.max(1, keepdim=True)[1] # get the index of the max log-probability

    #imgOut1 = torch.Tensor(1, 424, 424)
    #imgOut1[0] = output[0][0]

    #imgOut1 = imgOut1.cpu().float()
    imgPred = pred[0]
    imgPred = imgPred.cpu().float()
    print(target.size())
    imgTarget = torch.FloatTensor(1, 424, 424)
    imgTarget[0] = target[0]
    imgTarget = imgTarget.cpu().float()
    #saveImage(imgOut1, 'output1.png')
    saveImage(imgPred, 'prediction.png')
    saveImage(imgTarget, 'target.png')
    #showOutput(imgOut1)
    showOutput(imgPred)
    showOutput(imgTarget)

    '''