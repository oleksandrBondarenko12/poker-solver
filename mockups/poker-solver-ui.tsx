import React, { useState } from 'react';
import { ChevronRight } from 'lucide-react';

const HandRangeTable = ({ onCellClick, selectedHands = [] }) => {
  const ranks = ['A', 'K', 'Q', 'J', 'T', '9', '8', '7', '6', '5', '4', '3', '2'];
  
  const getCellType = (rankX, rankY) => {
    if (rankX === rankY) return 'pair';
    return rankY > rankX ? 'suited' : 'offsuit';
  };

  const getCellClassName = (type, isSelected) => {
    const baseClasses = 'w-12 h-12 text-center cursor-pointer text-sm';
    const bgColor = isSelected ? 'bg-yellow-400' : 'bg-gray-600';
    const textColor = type === 'pair' ? 'text-white' : 
                     type === 'suited' ? 'text-blue-300' : 'text-gray-400';
    return `${baseClasses} ${bgColor} ${textColor}`;
  };

  const getCellText = (rankX, rankY, type) => {
    if (type === 'pair') return `${rankX}${rankX}`;
    if (type === 'suited') return `${rankX}${rankY}s`;
    return `${rankX}${rankY}o`;
  };

  return (
    <table className="border-collapse bg-gray-700">
      <tbody>
        {ranks.map((rankY, i) => (
          <tr key={i}>
            {ranks.map((rankX, j) => {
              const type = getCellType(rankX, rankY);
              const hand = getCellText(rankX, rankY, type);
              const isSelected = selectedHands.includes(hand);
              
              return (
                <td 
                  key={`${i}-${j}`}
                  className={getCellClassName(type, isSelected)}
                  onClick={() => onCellClick?.(hand)}
                >
                  {hand}
                </td>
              );
            })}
          </tr>
        ))}
      </tbody>
    </table>
  );
};

const RangeSelector = ({ title, onClose }) => {
  const [rangeText, setRangeText] = useState(
    'QQ:0.5,JJ:0.75,TT,99,88,77,66,55,44,33,22,AKo:0.25,AQs,AQo:0.75,AJs,AJo:0.75,ATs,ATo:0.75,A9s,A8s,A7s,A6s,A5s,A4s,A3s,A2s,KQ,KJ,KTs,KTo:0.5,K9s,K8s,K7s,K6s,K5s,K4s:0.5,K3s:0.5,K2s:0.5,QJ,QTs,Q9s,Q8s,Q7s,JTs,JTo:0.5,J9s,J8s,T9s,T8s,T7s,98s,97s,96s,87s,86s,76s,75s,65s,64s,54s,53s,43s'
  );
  const [selectedHands, setSelectedHands] = useState([]);

  const handleCellClick = (hand) => {
    setSelectedHands(prev => 
      prev.includes(hand) 
        ? prev.filter(h => h !== hand)
        : [...prev, hand]
    );
  };

  return (
    <div className="bg-gray-900 p-4 rounded-lg">
      <h2 className="text-xl text-gray-300 mb-4">{title}</h2>
      
      <div className="mb-4">
        <textarea
          className="w-full bg-gray-800 text-gray-100 p-2 rounded resize-none"
          rows={3}
          value={rangeText}
          onChange={(e) => setRangeText(e.target.value)}
        />
      </div>

      <div className="flex items-center space-x-2 mb-4">
        <span className="text-gray-300">.000</span>
        <input
          type="range"
          className="flex-grow h-2 bg-gray-700 rounded-lg appearance-none"
          min="0"
          max="1000"
          step="1"
        />
      </div>

      <div className="flex space-x-2 mb-4">
        <button className="px-4 py-2 bg-blue-600 text-white rounded text-sm">Confirm</button>
        <button className="px-4 py-2 bg-gray-700 text-white rounded text-sm">Cancel</button>
        <button className="px-4 py-2 bg-gray-700 text-white rounded text-sm">Clear range</button>
        <button className="px-4 py-2 bg-gray-700 text-white rounded text-sm">Export Range</button>
        <button className="px-4 py-2 bg-gray-700 text-white rounded text-sm">Import Range</button>
        <button className="px-4 py-2 bg-gray-700 text-white rounded text-sm">Open Range Folder</button>
      </div>

      <div className="overflow-auto">
        <HandRangeTable 
          onCellClick={handleCellClick}
          selectedHands={selectedHands}
        />
      </div>

      <div className="mt-4 bg-gray-800 p-2">
        <div className="flex items-center text-gray-300">
          <ChevronRight className="w-4 h-4" />
          <span>/</span>
        </div>
      </div>
    </div>
  );
};

const StreetBettingControls = ({ title, defaultBetSize = 50, defaultRaiseSize = 60 }) => {
  return (
    <div className="space-y-2">
      <h3 className="text-sm font-medium">{title}</h3>
      <div className="space-y-2">
        <div className="flex items-center space-x-2">
          <span className="text-sm w-20">Bet Sizes:</span>
          <input 
            type="text" 
            className="flex-1 bg-gray-700 rounded p-1 text-sm"
            defaultValue={defaultBetSize}
          />
          <span className="text-sm">%</span>
        </div>
        <div className="flex items-center space-x-2">
          <span className="text-sm w-20">Raise Sizes:</span>
          <input 
            type="text" 
            className="flex-1 bg-gray-700 rounded p-1 text-sm"
            defaultValue={defaultRaiseSize}
          />
          <span className="text-sm">%</span>
        </div>
        <label className="flex items-center space-x-2">
          <input type="checkbox" className="rounded" defaultChecked />
          <span className="text-sm">Add AllIn</span>
        </label>
      </div>
    </div>
  );
};

const PokerSolverUI = () => {
  const [activeSelector, setActiveSelector] = useState(null);

  return (
    <div className="min-h-screen bg-gray-900 text-gray-100">
      {/* Header */}
      <div className="bg-gray-800 p-2">
        <span className="text-sm font-medium">Solver strategy parameters</span>
      </div>

      <div className="p-4 space-y-4">
        {/* Range Selection Buttons */}
        <div className="grid grid-cols-2 gap-4">
          <button 
            className="bg-gray-800 p-2 rounded text-left"
            onClick={() => setActiveSelector('ip')}
          >
            Select IP
          </button>
          <button 
            className="bg-gray-800 p-2 rounded text-left"
            onClick={() => setActiveSelector('oop')}
          >
            Select OOP
          </button>
        </div>

        {/* Board Input */}
        <div className="bg-gray-800 p-4 rounded">
          <h2 className="text-sm font-medium mb-2">Board</h2>
          <input 
            type="text" 
            className="w-full bg-gray-700 rounded p-2"
            defaultValue="5d 9h 4s"
          />
          <button className="mt-2 w-full text-center bg-gray-700 p-2 rounded text-sm">
            Select Board Card
          </button>
        </div>

        {/* IP Street Controls */}
        <div className="grid grid-cols-3 gap-4">
          <div className="bg-gray-800 p-4 rounded">
            <StreetBettingControls title="Flop IP" />
          </div>
          <div className="bg-gray-800 p-4 rounded">
            <StreetBettingControls title="Turn IP" />
          </div>
          <div className="bg-gray-800 p-4 rounded">
            <StreetBettingControls title="River IP" />
          </div>
        </div>

        <button className="w-full bg-gray-700 p-2 rounded text-sm">
          Copy from IP to OOP
        </button>

        {/* OOP Street Controls */}
        <div className="grid grid-cols-3 gap-4">
          <div className="bg-gray-800 p-4 rounded">
            <StreetBettingControls title="Flop OOP" />
          </div>
          <div className="bg-gray-800 p-4 rounded">
            <StreetBettingControls title="Turn OOP" />
          </div>
          <div className="bg-gray-800 p-4 rounded">
            <StreetBettingControls title="River OOP" />
          </div>
        </div>

        {/* Solver Configuration */}
        <div className="grid grid-cols-3 gap-4">
          <div className="flex items-center space-x-2">
            <span className="text-sm">raise limit:</span>
            <input 
              type="number" 
              className="w-16 bg-gray-700 rounded p-1"
              defaultValue={3}
            />
            <span className="text-sm">times</span>
          </div>
          <div className="flex items-center space-x-2">
            <span className="text-sm">Pot:</span>
            <input 
              type="number" 
              className="w-16 bg-gray-700 rounded p-1"
              defaultValue={50}
            />
          </div>
          <div className="flex items-center space-x-2">
            <span className="text-sm">Effective Stack:</span>
            <input 
              type="number" 
              className="w-24 bg-gray-700 rounded p-1"
              defaultValue={200}
            />
          </div>
        </div>

        {/* Advanced Options */}
        <div className="grid grid-cols-2 gap-4">
          <div className="flex items-center space-x-2">
            <span className="text-sm">allin threshold:</span>
            <input 
              type="number" 
              className="w-20 bg-gray-700 rounded p-1"
              defaultValue={0.67}
            />
          </div>
          <label className="flex items-center space-x-2">
            <input type="checkbox" className="rounded" defaultChecked />
            <span className="text-sm">use isomorphism</span>
          </label>
        </div>

        {/* Solver Controls */}
        <div className="space-y-2">
          <button className="w-full bg-gray-700 p-2 rounded text-sm">
            Build Tree
          </button>
          <button className="w-full bg-gray-600 p-2 rounded text-sm">
            Estimate Solving Memory
          </button>
          <div className="grid grid-cols-2 gap-4">
            <div className="flex items-center space-x-2">
              <span className="text-sm">Iterations:</span>
              <input 
                type="number" 
                className="flex-1 bg-gray-700 rounded p-1"
                defaultValue={200}
              />
              <span className="text-sm">times</span>
            </div>
            <div className="flex items-center space-x-2">
              <span className="text-sm">stop solving when reach</span>
              <input 
                type="number" 
                className="w-16 bg-gray-700 rounded p-1"
                defaultValue={0.5}
              />
              <span className="text-sm">% exploitability</span>
            </div>
          </div>
          <div className="flex items-center space-x-2">
            <span className="text-sm">log interval:</span>
            <input 
              type="number" 
              className="w-16 bg-gray-700 rounded p-1"
              defaultValue={10}
            />
            <span className="text-sm">threads:</span>
            <input 
              type="number" 
              className="w-16 bg-gray-700 rounded p-1"
              defaultValue={8}
            />
          </div>
        </div>

        {/* Action Buttons */}
        <div className="grid grid-cols-3 gap-4">
          <button className="bg-gray-700 p-2 rounded text-sm">Start solving</button>
          <button className="bg-gray-700 p-2 rounded text-sm">Stop solving</button>
          <button className="bg-gray-700 p-2 rounded text-sm">Show Result</button>
        </div>

        <button className="w-full bg-gray-700 p-2 rounded text-sm">
          Clear Log
        </button>
      </div>

      {/* Range Selector Modal */}
      {activeSelector && (
        <div className="fixed inset-0 bg-black bg-opacity-50 flex items-center justify-center">
          <div className="bg-gray-900 rounded-lg max-w-5xl w-full max-h-[90vh] overflow-auto">
            <RangeSelector 
              title={`RangeSelector - ${activeSelector.toUpperCase()}`}
              onClose={() => setActiveSelector(null)}
            />
          </div>
        </div>
      )}
    </div>
  );
};

export default PokerSolverUI;